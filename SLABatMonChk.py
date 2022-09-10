#!/usr/bin/env python3
#Filename: SLABatMonChk.py
#
# Check the SLA Battery Monitor CRCs.

filespecBatData = "./SLABatMon/SLABatMon-v3-ResultsCap.txt"

# Calculate a Dallas/Maxim CRC8 checksum. Literally, the Arduino code translated...
def calcCRC8(data2Check = ""):
    chksumCRC8 = 0
    for character in data2Check:
        dataByte = ord(character) # Get the ASCII value of the byte to be processed.
        for bitCounter in range(8):
            sum = ((dataByte ^ chksumCRC8) & 1)
            chksumCRC8 >>= 1
            if sum:
                chksumCRC8 ^= 0x8c
            dataByte >>= 1
    return chksumCRC8

print("======================================")
print("| SLA Battery Monitor Data CRC Check |")
print("======================================")
print()
print("Battery Data file:", filespecBatData)
print()

lineCounter = 0
myCRC8Fails = 0
# Open up the Battery results data file.
with open(filespecBatData, 'r') as myBatData:
    for line in myBatData:
        line = line.strip()
        lastDelimiter = line.rfind(",")
        vData = line[:lastDelimiter]
        if lineCounter == 0:
            print("Data Headings:", vData)
            print("Checking CRCs...")
        else:
            print(" #%s" % lineCounter, end = '')
            vCRC8 = int(line[lastDelimiter + 1:])
            myCRC8 = calcCRC8(vData)
            if myCRC8 != vCRC8:
                myCRC8Fails += 1
                print("!")
                print("CRC8 Fail: Line %d = CRC#%s - %s (%d!%d)" % (lineCounter + 1, lineCounter, vData, vCRC8, myCRC8))
        lineCounter += 1

print()
print()
print("Number of CRCs checked :", lineCounter - 1)
print("  Number of failed CRCs:", myCRC8Fails)
print()

# EOF