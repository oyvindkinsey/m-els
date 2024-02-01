#include <Wire.h>
#include <CommandParser.h>

#define RXD2 16
#define TXD2 17

typedef CommandParser<> MyCommandParser;
MyCommandParser parser;

byte ADDRESS = 0x1;
#define VERSION_OFFSET 0x0
#define GEARS_OFFSET 0x3
#define MODE_OFFSET 0x5
#define RPM_OFFSET 0x1
#define POS_OFFSET 0x6
#define VERSION 1

bool initialized = false;

void read_info() {
  Wire.beginTransmission(ADDRESS);
  Wire.write(GEARS_OFFSET);
  Wire.endTransmission(false);
  Wire.requestFrom(ADDRESS, 2, true);  // request 2 bytes
  byte num = Wire.read();              // read the driving gear
  byte denom = Wire.read();            // read the driven gear
  Serial.print("Gears: ");
  Serial.print(num, DEC);
  Serial.print("/");
  Serial.println(denom, DEC);
  Wire.endTransmission();

  Wire.beginTransmission(ADDRESS);
  Wire.write(MODE_OFFSET);
  Wire.endTransmission(false);
  Wire.requestFrom(ADDRESS, 1, true);
  byte mode = Wire.read();
  Serial.print("Mode: ");
  Serial.println(mode);
  Wire.endTransmission();

  Wire.beginTransmission(ADDRESS);
  Wire.write(RPM_OFFSET);
  Wire.endTransmission(false);
  Wire.requestFrom(ADDRESS, 2, true);     // request 2 bytes
  byte rpm_l = Wire.read();               // read the lsb of the 16 bit number
  byte rpm_h = Wire.read();               // read the msb of the 16 bit number
  uint16_t rpm = ((rpm_h << 8) | rpm_l);  // get the 16 bit number
  Serial.print("RPM: ");
  Serial.println(rpm, DEC);
  Wire.endTransmission();

  Wire.beginTransmission(ADDRESS);
  Wire.write(POS_OFFSET);
  Wire.endTransmission(false);
  Wire.requestFrom(ADDRESS, 2, true);     // request 2 bytes
  byte pos_l = Wire.read();               // read the lsb of the 16 bit number
  byte pos_h = Wire.read();               // read the msb of the 16 bit number
  uint16_t pos = ((pos_h << 8) | pos_l);  // get the 16 bit number
  Serial.print("Position: ");
  Serial.println(pos, DEC);
  Wire.endTransmission();
}

void set_gear(byte num, byte denom) {
  Wire.beginTransmission(ADDRESS);
  Wire.write(GEARS_OFFSET);
  Wire.write(num);
  Wire.write(denom);
  Wire.endTransmission();
}

void set_mode(byte mode) {
  Wire.beginTransmission(ADDRESS);
  Wire.write(MODE_OFFSET);
  Wire.write(mode);
  Wire.endTransmission();
}

// read from the registry
void cmd_reg(MyCommandParser::Argument *args, char *response) {
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

// set mode
void cmd_mode(MyCommandParser::Argument *args, char *response) {
  set_mode(args[0].asUInt64);
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
  parser.registerCommand("gear", "ii", &cmd_gear);
  parser.registerCommand("reg", "ii", &cmd_reg);
  parser.registerCommand("read", "", &cmd_read);
  parser.registerCommand("mode", "i", &cmd_mode);
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
  set_gear(1, 10);
  initialized = true;

  setup_commands();
}

void loop() {
  if (!initialized) {
    return;
  }
  read_command();
  relay_debug();
}
