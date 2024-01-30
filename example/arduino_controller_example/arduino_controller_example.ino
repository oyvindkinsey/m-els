#include <Wire.h>

byte ADDRESS = 0x1;
byte VERSION_OFFSET = 0x0;
byte FLAGS_MASK_OFFSET = 0x3;
byte FLAGS_OFFSET = 0x4;
byte GEARS_OFFSET = 0x5;
byte RPM_OFFSET = 0x1;
byte VERSION = 1;
bool initialized = false;

void setup() {
  Serial.begin(115200);
  Wire.setClock(400000);
  Wire.begin();

  // Start by reading the version
  Serial.println("Reading version:");
  Wire.beginTransmission(ADDRESS);
  Wire.write(VERSION_OFFSET);
  Wire.endTransmission(false);

  Wire.requestFrom(ADDRESS, 1, true);  // request 1 byte
  byte version = Wire.read();          // read the flags
  Serial.println(version, DEC);
  Wire.endTransmission();

  // Initialize driver
  Wire.beginTransmission(ADDRESS);
  Wire.write(FLAGS_MASK_OFFSET);  //move to register 0x2
  Wire.write(0b11000000);         // set the flags mask for the bits we intend to write
  Wire.write(0x80 | 0x40 | 0x1);  // enable stepper and rpm (and set one ignored bit)
  Wire.write(1);                  // set the driving gear to 1
  Wire.write(10);                 // set the driven gear to 10
  int error = Wire.endTransmission();
  if (error != 0) {
    Serial.println("Failed to initialize");
  } else {
    if (version == VERSION) {
      initialized = true;
    } else {
      Serial.println("Expected version:");
      Serial.println(VERSION);
    }
  }
}

void loop() {
  if (!initialized) {
    return;
  }
  Serial.println("Reading flags:");
  Wire.beginTransmission(ADDRESS);
  Wire.write(FLAGS_OFFSET);
  Wire.endTransmission(false);

  Wire.requestFrom(ADDRESS, 1, true);  // request 1 byte
  byte flags = Wire.read();            // read the flags
  Serial.println(flags, BIN);
  Wire.endTransmission();

  Serial.println("Reading gears");
  Wire.beginTransmission(ADDRESS);
  Wire.write(GEARS_OFFSET);
  Wire.endTransmission(false);
  Wire.requestFrom(ADDRESS, 2, true);  // request 2 bytes
  byte num = Wire.read();              // read the driving gear
  byte denom = Wire.read();            // read the driven gear
  Serial.print(num, DEC);
  Serial.print("/");
  Serial.println(denom, DEC);
  Wire.endTransmission();

  Serial.println("Reading RPM");
  Wire.beginTransmission(ADDRESS);
  Wire.write(RPM_OFFSET);
  Wire.endTransmission(false);
  Wire.requestFrom(ADDRESS, 2, true);  // request 2 bytes
  byte lsb = Wire.read();              // read the lsb of the 16 bit number
  byte msb = Wire.read();              // read the msb of the 16 bit number
  uint16_t rpm = ((msb << 8) | lsb);   // get the 16 bit number
  Serial.println(rpm, DEC);
  Wire.endTransmission();

  delay(1000);
}