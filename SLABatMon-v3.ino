/*!
 * Sealed Lead Acid Battery Monitor.
 * Data shown on an I2C 20X4 LCD display and sent to the console for logging/graphing.
 * 
 * 01.00.00 First Public release.
 * 02.00.00 ADC callibration performed before every ADC read.
 *          Calibrated ADCRef50v reading sent to the serial console.
 * 03.00.00 Added a CRC8 checksum to the console data stream.
 *          Added a watchdog timer reset. Note that Nanos need the new bootloader!
 * 
 * (c) 2022 Ian Neill, arduino@binaria.co.uk
 * Licence: GPLv3
 * 
 * https://github.com/duinoWitchery/hd44780
 * https://skillbank.co.uk/arduino/measure.htm
 * https://baltazarstudios.com/accuracy-of-arduinos-adc/
 * https://crccalc.com/?method=crc8
 */

// Libraries.
#include <Wire.h>                           // The Arduino I2C library.
#include <hd44780.h>                        // HD44780 via I2C LCD display library (by Bill Perry, installed via Library Manager).
#include <hd44780ioClass/hd44780_I2Cexp.h>  // I2C expander I/O class header (installed with Bill Perry's hd44780 Library).
#include <Watchdog.h>                       // A simple watchdog library (by Peter Polidoro, installed via Library Manager).

// I/O defines.
#define BAT1VMON A0                         // Battery 1 monitor input.
#define BAT2VMON A1                         // Battery 2 monitor input.
#define BAT3VMON A2                         // Battery 3 monitor input.
#define BAT4VMON A3                         // Battery 4 monitor input.
#define AVRFVMON A7                         // Analog voltage reference monitor.
#define BKLGTCTL 2                          // LCD display backlight control.
#define HB_LED   13                         // Heartbeat LED.

// ADC read defines.
#define ADCREADYDELAY  5
#define ADCSAMPLES     8
#define ADCSAMPLEDELAY 1

// Reference voltage define.
#define AVREF 3.3                           // Reference voltage on pin AVRFVMON (A7).
#define ADCLEVELS 1024                      // Number of levels for a 10bit ADC.

// My 100K/50K voltage divider ratio.
#define VDIVRATIO 3.0

// I2C LCD display defines - 20 character X 4 line LCD display at address at 0x27.
#define LCD_COLS 20
#define LCD_ROWS 4
#define LCD_I2C_ADDRESS 0x27

// Initialise the LCD display on the I2C interface, SDA = A4, SCL = A5.
hd44780_I2Cexp myLCD(LCD_I2C_ADDRESS);

// Transmit data buffer.
#define TXBUFFERMAX 32                      // The maximum size of the data buffer in which voltage readings are stored before sending.
char txBuffer[TXBUFFERMAX + 1];             // A character array variable to hold voltage reading data characters.

// Code loop job defines.
#define JOB1CYCLE 300000                    // Job 1 execution cycle: 5.0m - Get the data: Read the analog inputs.
#define JOB2CYCLE 300000                    // Job 2 execution cycle: 5.0m - Share the results: Send the data to the console and LCD.
#define JOB3CYCLE 1000                      // Job 3 execution cycle: 1.0m - Backlight Control: Turn the LCD backlight ON/OFF.
#define JOB9CYCLE 500                       // Job 9 execution cycle: 0.5s - Toggle the heartbeat LED.

#define ADCREADPERIOD (JOB1CYCLE / 60000.0) // How many minutes between ADC reads.

// Watchdog initialisation.
Watchdog cerberous;

void setup() {
  // Set up the I/O pins.
  pinMode(BAT1VMON, INPUT);
  pinMode(BAT2VMON, INPUT);
  pinMode(BAT3VMON, INPUT);
  pinMode(BAT4VMON, INPUT);
  pinMode(AVRFVMON, INPUT);
  pinMode(BKLGTCTL, INPUT_PULLUP);          // LCD backlight ON/OFF.
  pinMode(HB_LED, OUTPUT);                  // The builtin LED.
  //Start the serial output at 9600 baud.
  Serial.begin(9600);
  // Initialise the I2C LCD display.
  myLCD.begin(LCD_COLS, LCD_ROWS);          // Start and clear the LCD display.
  showLCDStaticData();                      // Show the static LCD data.
  // Send logging/graphing labels to the console.
  Serial.println("ADCRef50v,Battery-1,Battery-2,Battery-3,Battery-4,CRC8");
  // Start the watchdog.
  cerberous.enable();                       // The default watchdog timeout period is 1000ms.
}

void loop() {
  // Set up variables.
  static bool hbStatus = LOW;               // Heartbeat LED status.
  static bool blStatus = HIGH;              // LCD backlight status.
  static unsigned long adcReadCounter = 0;  // The number of ADC reads is used to calculate the sketch running time.
  float adcOneLvl, adcRef50v, adcFactor, myVoltsB1, myVoltsB2, myVoltsB3, myVoltsB4;
  // Record the current time. When a single timeNow is used for all jobs it ensures they are synchronised.
  unsigned long timeNow = millis();
  // Job variables.
  static unsigned long timeMark1 = 0;       // Last time Job 1 was executed.
  static unsigned long timeMark2 = 0;       // Last time Job 2 was executed.
  static unsigned long timeMark3 = 0;       // Last time Job 3 was executed.
  static unsigned long timeMark9 = 0;       // Last time Job 9 was executed.
  // Job 1 - Get the data: Read the analog inputs.
  if (timeNow - timeMark1 >= JOB1CYCLE) {
    timeMark1 = timeNow;
    // Calculate the ADC Vref calibration and correction factor.
    adcOneLvl = AVREF / getADCReading(AVRFVMON);
    adcRef50v = adcOneLvl * ADCLEVELS;
    adcFactor = adcOneLvl * VDIVRATIO;
    // Read the analog inputs and calculate the battery voltages.
    myVoltsB1 = adcFactor * getADCReading(BAT1VMON);
    myVoltsB2 = adcFactor * getADCReading(BAT2VMON);
    myVoltsB3 = adcFactor * getADCReading(BAT3VMON);
    myVoltsB4 = adcFactor * getADCReading(BAT4VMON);
    adcReadCounter += 1;
  }
  // Job 2 - Share the results: Send the data to the console and LCD.
  if (timeNow - timeMark2 >= JOB2CYCLE) {
    timeMark2 = timeNow;
    // Output battery voltages.
    sendConsoleData(adcRef50v, myVoltsB1, myVoltsB2, myVoltsB3, myVoltsB4);
    showLCDPeriodData(adcReadCounter);
    showLCDBatteryData(myVoltsB1, myVoltsB2, myVoltsB3, myVoltsB4);
  }
  // Job 3 - Backlight Control: Turn the LCD backlight ON/OFF.
  if (timeNow - timeMark3 >= JOB3CYCLE) {
    timeMark3 = timeNow;
    // Read the LCD backlight control input act on it.
    if (digitalRead(BKLGTCTL)) {
      if (not blStatus) {
        myLCD.backlight();
        blStatus = HIGH;
      }
    }
    else {
      if (blStatus) {
        myLCD.noBacklight();
        blStatus = LOW;
      }
    }
  }
  // Job 9 - Toggle the heartbeat LED.
  if (timeNow - timeMark9 >= JOB9CYCLE) {
    timeMark9 = timeNow;
    // Toggle the heartbeat LED.
    hbStatus = !hbStatus;
    digitalWrite(HB_LED, hbStatus);
    // Reset the watchdog.
    cerberous.reset();
  }
}

int getADCReading(byte pinNumber) {
  int adcAverage;
  float adcTotal = 0;
  // Dummy read for the multiplexed ADC to connect and become ready for accurate reading.
  analogRead(pinNumber);
  delay(ADCREADYDELAY);
  for (byte counter = 0; counter < ADCSAMPLES; counter++) {
    // Accumulate the readings, adding 0.5 to average the rounding down caused by the ADC quantisation.
    adcTotal += (analogRead(pinNumber) + 0.5);
    delay(ADCSAMPLEDELAY);
  }
  // Average the reading and add 0.5 so that the rounding is to the nearest whole number, up or down.
  adcAverage = (adcTotal / ADCSAMPLES) + 0.5;
  return adcAverage;  
}

void sendConsoleData(float adcRef33v, float b1Voltage, float b2Voltage, float b3Voltage, float b4Voltage) {
  byte chksumCRC8;
  char bRefStr[8], b1VStr[8], b2VStr[8], b3VStr[8], b4VStr[8];
  // The voltages are needed as strings.
  dtostrf(adcRef33v, 4, 2, bRefStr); 
  dtostrf(b1Voltage, 4, 2, b1VStr);
  dtostrf(b2Voltage, 4, 2, b2VStr);
  dtostrf(b3Voltage, 4, 2, b3VStr);
  dtostrf(b4Voltage, 4, 2, b4VStr);
  // Construct the voltage data string in the txbuffer.
  sprintf(txBuffer, "%s,%s,%s,%s,%s", bRefStr, b1VStr, b2VStr, b3VStr, b4VStr);
  // Calculate the CRC8 checksum of the txBuffer.
  chksumCRC8 = calcCRC8((byte*)txBuffer); // Cast the char array pointer to a byte array pointer.
  // Print the results.
  Serial.print(txBuffer);
  // Add the CRC8 checksum to the end.
  Serial.print(",");
  Serial.println(chksumCRC8);
}

void showLCDStaticData() {
  myLCD.setCursor(0, 0);
  myLCD.print("SLA Battery Voltages");
  myLCD.setCursor(1, 1);
  myLCD.print("Time: ---d --h --m");
  myLCD.setCursor(0, 2);
  myLCD.print("B1:--.--v");
  myLCD.setCursor(11, 2);
  myLCD.print("B2:--.--v");
  myLCD.setCursor(0, 3);
  myLCD.print("B3:--.--v");
  myLCD.setCursor(11, 3);
  myLCD.print("B4:--.--v");
}

void showLCDPeriodData(unsigned long adcReadCounter) {
  unsigned long totalMins = adcReadCounter * ADCREADPERIOD;
  byte pMins  = totalMins % 60;                       // Modulus mins, 0-59.
  byte pHours = (totalMins / 60) % 24;                // Modulus hours, 0-23.
  unsigned int pDays = (totalMins / 60 / 24) % 1000;  // Modulus days, 0-999.
  char lcdMessage[13];                                // 12 characters and /0.
  // Build the formatted message for the LCD display.
  sprintf(lcdMessage, "%03dd %02dh %02dm", pDays, pHours, pMins);
  myLCD.setCursor(7, 1);
  myLCD.print(lcdMessage);
}

void showLCDBatteryData(float b1Voltage, float b2Voltage, float b3Voltage, float b4Voltage) {
  myLCD.setCursor(3, 2);
  showLCDVoltage(b1Voltage);
  myLCD.setCursor(14, 2);
  showLCDVoltage(b2Voltage);
  myLCD.setCursor(3, 3);
  showLCDVoltage(b3Voltage);
  myLCD.setCursor(14, 3);
  showLCDVoltage(b4Voltage);
}

void showLCDVoltage(float bVoltage) {
  if (bVoltage >= 0 and bVoltage < 100) {
    if (bVoltage < 10) {
      myLCD.print("0");
    }
    myLCD.print(bVoltage);
  }
  else {
    myLCD.print("--.--");
  }
}

// Calculate the CRC8 checksum of a null terminated character array.
// Based on the CRC8 formulas by Dallas/Maxim (GNU GPL 3.0 license).
byte calcCRC8(byte* dataBuffer) {
  // Initialise the CRC8 checksum.
  byte chksumCRC8 = 0;
  // While the byte to be process is not the null terminator.
  while(*dataBuffer != '\0') {
    byte currentByte = *dataBuffer; // Get the byte to be processed.
    // Process each bit of the byte. 
    for (byte bitCounter = 0; bitCounter < 8; bitCounter++) {
        byte sum = (chksumCRC8 ^ currentByte) & 0x01;
        chksumCRC8 >>= 1;
        if (sum) {
           chksumCRC8 ^= 0x8C;
        }
        currentByte >>= 1;
     }
     dataBuffer++;
  }
  return chksumCRC8;
}

// EOF
