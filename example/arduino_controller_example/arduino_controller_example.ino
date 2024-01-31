#include <Wire.h>
#include <CommandParser.h>

#define RXD2 16
#define TXD2 17

typedef CommandParser<> MyCommandParser;
MyCommandParser parser;

byte ADDRESS = 0x1;
byte VERSION_OFFSET = 0x0;
byte FLAGS_MASK_OFFSET = 0x3;
byte FLAGS_OFFSET = 0x4;
byte GEARS_OFFSET = 0x5;
byte RPM_OFFSET = 0x1;
byte FLAGS_STEPPER_ENABLED_MASK = 0b10000000;

#define VERSION 1

bool initialized = false;

void read_info() {
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
}

void set_flags(byte mask, byte flags) {
  Wire.beginTransmission(ADDRESS);
  Wire.write(FLAGS_MASK_OFFSET);
  Wire.write(mask);
  Wire.write(flags);
  Wire.endTransmission();
}

void set_gear(byte num, byte denom) {
  Wire.beginTransmission(ADDRESS);
  Wire.write(GEARS_OFFSET);
  Wire.write(num);
  Wire.write(denom);
  Wire.endTransmission();
}

// read from the registry
void cmd_read_reg(MyCommandParser::Argument *args, char *response) {
  char offset = args[0].asUInt64;
  auto length = args[1].asUInt64;

  Wire.beginTransmission(ADDRESS);
  Wire.write(offset);
  Wire.endTransmission(false);
  Wire.requestFrom(ADDRESS, length, true);  // request 2 bytes
  byte val;
  Serial.print("Values at offset ");
  Serial.println(offset, DEC);
  while (Wire.available() != 0) {
    val = Wire.read();
    Serial.println(val);
  }
  Wire.endTransmission();
}

// set flags
void cmd_flags(MyCommandParser::Argument *args, char *response) {
  set_flags(0xFF, args[0].asUInt64);
  read_info();
}

// set stepper enabled
void cmd_step_en(MyCommandParser::Argument *args, char *response) {
  set_flags(FLAGS_STEPPER_ENABLED_MASK, args[0].asUInt64 ? 0xFF : 0x0);
  read_info();
}

// set the gearing
void cmd_gear(MyCommandParser::Argument *args, char *response) {
  set_gear(args[0].asUInt64, args[1].asUInt64);
  read_info();
}

// read information
void cmd_read(MyCommandParser::Argument *args, char *response) {
  read_info();
}

void setup_commands() {
  parser.registerCommand("step_en", "i", &cmd_step_en);
  parser.registerCommand("gear", "ii", &cmd_gear);
  parser.registerCommand("reg", "ii", &cmd_read_reg);
  parser.registerCommand("flags", "i", &cmd_flags);
  parser.registerCommand("read", "", &cmd_flags);
}

void read_command() {
  if (Serial.available()) {
    char line[128];
    size_t lineLength = Serial.readBytesUntil('\n', line, 127);
    line[lineLength] = '\0';

    char response[MyCommandParser::MAX_RESPONSE_SIZE];
    parser.processCommand(line, response);
    Serial.println(response);
  }
}

void relay_debug() {
  if (Serial2.available()) {
    String stm = Serial2.readString();
    Serial.print("STM: ");
    Serial.println(stm);
  }
}

void setup() {
  // for communication with user
  Serial.begin(115200);
  // to relay debug messages from driver
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);

  Wire.setClock(400000);
  Wire.begin();

  // Start by reading the version
  Serial.println("Reading version:");
  Wire.beginTransmission(ADDRESS);
  Wire.write(VERSION_OFFSET);
  Wire.endTransmission(false);
  Wire.requestFrom(ADDRESS, 1, true);  // request 1 byte
  byte version = Wire.read();          // read the version
  Wire.endTransmission();
  Serial.println(version);

  if (version != VERSION) {
    Serial.println("Wrong version");
    return;
  }

  // Initialize driver
  Wire.beginTransmission(ADDRESS);
  Wire.write(FLAGS_MASK_OFFSET);
  Wire.write(0b11000000);         // set the flags mask for the bits we intend to write
  Wire.write(0x80 | 0x40 | 0x1);  // enable stepper and rpm (and set one ignored bit)
  Wire.write(1);                  // set the driving gear to 1
  Wire.write(10);                 // set the driven gear to 10
  int error = Wire.endTransmission();
  if (error != 0) {
    Serial.println("Failed to initialize");
  } else {
    initialized = true;
  }

  setup_commands();
}

void loop() {
  if (!initialized) {
    return;
  }
  read_command();
  relay_debug();
}
