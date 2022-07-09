/*!
 * AVRef 3.3v Tests.
 * Using the 3.3v reference to improve the accuracy of analog reads that use the 5v reference.
 * 
 * (c) 2022 Ian Neill, arduino@binaria.co.uk
 * Licence: GPLv3
 * 
 * https://skillbank.co.uk/arduino/measure.htm
 */

// I/O defines.
#define BAT1VMON A0                         // Battery 1 monitor input.
#define BAT2VMON A1                         // Battery 2 monitor input.
#define AVRFVMON A7                         // Analog voltage reference monitor.
#define HB_LED   13                         // Heartbeat LED.

// ADC read defines.
#define ADCREADYDELAY  5
#define ADCSAMPLES     8
#define ADCSAMPLEDELAY 1

// Reference voltage defines.
#define AVREF 3.3                           // Reference voltage on pin AVRFVMON (A7).
#define ADCMAXVALUE 1023                    // For an Arduino 10bit ADC.

// Code loop job defines.
#define JOB1CYCLE 5000                      // Job 1 execution cycle: 5.0s - Get the data: Read the analog inputs.
#define JOB2CYCLE 5000                      // Job 2 execution cycle: 5.0s - Share the results: Send the data to the console.
#define JOB9CYCLE 500                       // Job 9 execution cycle: 0.5s - Toggle the heartbeat LED.

// My 100K/50K voltage divider ratio.
#define VDIVRATIO 3.0

void setup() {
  // Set up the I/O pins.
  pinMode(BAT1VMON, INPUT);
  pinMode(BAT2VMON, INPUT);
  pinMode(AVRFVMON, INPUT);
  pinMode(HB_LED, OUTPUT);                  // The builtin LED.
  //Start the serial output at 9600 baud.
  Serial.begin(9600);
  // Console title.
  Serial.println("AV 3.3v Ref Tests");
  Serial.println("=================");
}

void loop() {
  // Set up variables.
  static bool hbStatus = LOW;               // Heartbeat LED status.
  int myAV33ADC;
  float myAV50Ref, myVoltsB1, myVoltsB2;
  // Record the current time. When a single timeNow is used for all jobs it ensures they are synchronised.
  unsigned long timeNow = millis();
  // Job variables.
  static unsigned long timeMark1 = 0;       // Last time Job 1 was executed.
  static unsigned long timeMark2 = 0;       // Last time Job 2 was executed.
  static unsigned long timeMark9 = 0;       // Last time Job 9 was executed.
  // Job 1 - Get the data: Read the analog inputs.
  if (timeNow - timeMark1 >= JOB1CYCLE) {
    timeMark1 = timeNow;
    // Read the analog inputs and calculate the battery voltages.
    myAV33ADC = getADCReading(AVRFVMON);                                    // Get the known 3.3v ADC value referenced to the unknown 5v value.
    myAV50Ref = (AVREF / myAV33ADC) * (ADCMAXVALUE + 1);                    // Work out what voltage the 5v supply value actually is.
    myVoltsB1 = (AVREF / myAV33ADC) * getADCReading(BAT1VMON) * VDIVRATIO;  // Use the calculated 5v value to work out the battery 1 voltage.
    myVoltsB2 = (AVREF / myAV33ADC) * getADCReading(BAT2VMON) * VDIVRATIO;  // Use the calculated 5v value to work out the battery 2 voltage.
  }
  // Job 2 - Share the results: Send the data to the console.
  if (timeNow - timeMark2 >= JOB2CYCLE) {
    timeMark2 = timeNow;
    // Output read and calculated data.
    Serial.print("Raw 3.3v reading  : ");
    Serial.println(myAV33ADC);
    Serial.print(" Calculated AV5Ref: ");
    Serial.print(myAV50Ref);
    Serial.println("v");
    Serial.print("  Battery 1 Volts : ");
    Serial.print(myVoltsB1);
    Serial.println("v");
    Serial.print("  Battery 2 Volts : ");
    Serial.print(myVoltsB2);
    Serial.println("v");
  }
  // Job 9 - Toggle the heartbeat LED.
  if (timeNow - timeMark9 >= JOB9CYCLE) {
    timeMark9 = timeNow;
    // Toggle the heartbeat LED.
    hbStatus = !hbStatus;
    digitalWrite(HB_LED, hbStatus);
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

//EOF
