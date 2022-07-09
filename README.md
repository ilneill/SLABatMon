# SLABatMon
Testing and Monitoring some UPS 12v Sealed Lead Acid Batteries.

Recently one of my home UPS's started complaining about it's battery. I have a number of UPS's, so this is not a new thing, as on average a UPS battery has about a 3yr life. Anyway, this got me thinking, how do I know these battries are bad and what makes a bad battery, bad?

Here is the simple hardware I built and the Arduino code I wrote to test and monitor these batteries, and my findings so far.

Bad battery characteristics I have found:
1. A 12v SLA car battery charger rejects it (fault light illuminated) and refuses to charge it.
2. When "fully charged", the battery voltage is less than 12.5v - 13.5v. Mine bad battery was a little over 11v.
3. After charging it does not hold the charge for long under little or no load. In less than 1 week my bad battery was down to under 5v!

My favourite part of the project was using the Arduino 3.3v supply as a reference voltage to calibrate the ADC measurements and voltage calculations.

You can see an explanation and demonstration here:
 - https://youtu.be/aRnLHJlChSg

WARNING: 12v SLA batteries are powerful current sources. Do not take chances, but take care instead!

## Fritzing hardward schematic:
![](SLABatMon_bb.png)

## LCD display in action:
![](WIN_20220704_19_37_20_Pro.jpg)
