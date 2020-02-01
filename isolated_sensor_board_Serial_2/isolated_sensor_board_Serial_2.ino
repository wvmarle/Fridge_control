// This code belongs to the isolated Arduino on Williams Fridge Control Board V.2 / V.2.1
//
// pH reading is from a second board, again isolated, to prevent the two sensors affecting one another.
//

#include <SoftwareSerial.h>

const uint8_t rxPin = 6;
const uint8_t txPin = 5;
SoftwareSerial mySerial = SoftwareSerial(rxPin, txPin);

const uint8_t CAPPOS = 2;
const uint8_t CAPNEG = 4;
const uint8_t ECPIN = 3;

void setup() {
  mySerial.begin(2400);                                     // Nice and slow or the ESP8266 can't follow...
  Serial.begin(9600);                                       // Read the other sensor board.
  initECSensor();
  initpHSensor();
}

uint16_t i = 0;
uint32_t lastTransmitData;
const uint32_t transmitInterval = 3000;

// Readings transmitted as:
// <Eec,Pph>
// with ec as discharge cycles (0-65535)
// ph as analog reading *32 (0-32767)

const uint32_t printInterval = 2000;

void loop() {
  uint32_t ECReading, pHReading;
  pHReading = readpHSensor();
  if (millis() - lastTransmitData > transmitInterval) {
    lastTransmitData += transmitInterval;
    ECReading = readECSensor();
    mySerial.print("<E");                                     // Transmit the data.
    mySerial.print(ECReading);
    mySerial.print(",P");
    mySerial.print(pHReading);
    mySerial.print(">");
  }
}
