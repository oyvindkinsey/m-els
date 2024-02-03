# Modular Electronic Leadscrew
m-els is a project for an electronic leadscrew that is focused on implementing a highly performant and flexible electronic leadscrew.
Its main design is a driver based on STM32, which implements the core translation from encoder input to stepper output, and which is controlled via I2C and select interrupts.

## Components
### Driver
This component is optimized for performance, and uses low level port manipulation, interrupts, and DMA for all data transfer.
Since this is free of having to handle input and displays, its resources can be used entirely on implementing core functionality without sacrificing support for high resolutions or high RPM.

#### Interface
The driver acts as an I2C slave, and has its 'gearing' set by writing to exposed registers. The driver also expose interrupt lines, where the interrupt type and related information can be read from respective registers, and subsequently cleared.

##### Registers

###### Information about the driver (INFO)
**Address Offset: 0x0**
|Offset|Type|Name|Description|  
|---|---|---|---|
|0x0|uint8_t|version|The protocol version this driver implements|

###### Initial configuration of the driver (CONFIGURATION)
**Address Offset: 0xA**
|Offset|Type|Name|Description|  
|---|---|---|---|
|0x0|uint16_t|encoder_resolution|The number of transitions per full rotation of the spindle.|
|0x2|uint16_t|stepper_resolution|The number of pulses needed for a full rotation of the leadscrew.|
|0x4|uint16_t|stepper_pulse_length_ns|Must be set above the minimum length required by the stepper.|
|0x6|uint16_t|stepper_change_dwell|Must be set above the minimum delay required by the stepper when changing directions.|
|0x8|uint8_t|stepper_flags|Flags for configuring the stepper. See separate table.|

**Stepper Flags**
|7|6|5|4|3|2|1|0|
|---|---|---|---|---|---|---|---|
|invert_step_pin|invert_dir_pin|||||||

###### Settings controlling the real-time operation of the driver (SETTINGS)
**Address Offset: 0x1E**
|Offset|Type|Name|Description|  
|---|---|---|---|
|0x0|uint8_t|mode|0 for disabled, 1 for synchronized motion, 2 for non-synchronized motion.|
|0x1|uint8_t|gear_num|The number of teeth on the virtual drive gear. Maximum 255.|
|0x2|uint8_t|gear_denom||The number of teeth on the virtual driven gear. Must be set less than or equal to gear_num.|

###### The observable state of the driver (STATE)
**Address Offset: 0x32**
|Offset|Type|Name|Description|  
|---|---|---|---|
|0x0|uint16_t|rpm|The RPM of the spindle.|
|0x2|uint16_t|pos|The current position of the encoder.|

##### Example

```cpp
#include <Wire.h>

void set_gear(byte num, byte denom) {
  Wire.beginTransmission(ADDRESS);
  Wire.write(SETTINGS_GEAR_NUM);
  Wire.write(num);
  Wire.write(denom);
  Wire.endTransmission();
}

void set_mode(byte mode) {
  Wire.beginTransmission(ADDRESS);
  Wire.write(SETTINGS_MODE);
  Wire.write(mode);
  Wire.endTransmission();
}

void read_info() {
  Wire.beginTransmission(ADDRESS);
  Wire.write(SETTINGS_BASE);
  Wire.endTransmission(false);
  Wire.requestFrom(ADDRESS, 3, true);

  byte mode = Wire.read();
  byte num = Wire.read();    // read the driving gear
  byte denom = Wire.read();  // read the driven gear
  Serial.print("Mode: ");
  Serial.println(mode);
  Serial.print("Gears: ");
  Serial.print(num, DEC);
  Serial.print("/");
  Serial.println(denom, DEC);
  Wire.endTransmission();

  Wire.beginTransmission(ADDRESS);
  Wire.write(STATE_BASE);
  Wire.endTransmission(false);
  Wire.requestFrom(ADDRESS, 4, true);
  uint16_t rpm = read_word();
  Serial.print("RPM: ");
  Serial.println(rpm, DEC);
  uint16_t pos = read_word();
  Serial.print("Position: ");
  Serial.println(pos, DEC);
  Wire.endTransmission();
}

void setup() {
  Serial.begin(115200);
  Wire.setClock(400000);
  Wire.begin();

  // Start by reading the version
  Serial.println("Reading version:");
  Wire.beginTransmission(ADDRESS);
  Wire.write(INFO_BASE + VERSION_OFFSET);
  Wire.endTransmission(false);

  Wire.requestFrom(ADDRESS, 1, true);
  byte version = Wire.read();
  Serial.println(version, DEC);
  Wire.endTransmission();

  // Initialize driver
  set_gear(4, 5);
  set_mode(1);
}

void loop() {
  read_info();
  delay(1000);
}
```
See https://github.com/oyvindkinsey/m-els/blob/master/example/arduino_controller_example/arduino_controller_example.ino for a complete example.
##### Interrupts
An interrupt will be raised whenever the flags change, such as during a fault situation, or when I/O triggers a state change.

##### I/O
In addition to I2C, certain operations can be triggered via external interrupt lines.
* E-Stop: Halts the driver immediately. Normally connected to ground. Triggers when positive or floating.
* Pendant A: Implementation dependent on mode.
* Pendant B: Implementation dependent on mode.

### Planned Features
* Set home
* Set left and right limit
* Set feed speed (for movement not synchronized to the spindle)
* Set rapid speed (for movement not synchronized to the spindle)
* Feed mode: Move at fixed speed 
* Rapid mode: Move at a fixed speed 
* Move to pos (in feed or rapid mode)
* Synchronized start (in threading mode) for single and multi-start threads

### User Interface
The above features are few, but provides the building bloks for more 'advanced' features to be built. The driver will maintain few, simple primitives, while the user interface is where higher order functionality is defined and implemented.
A user interface can be selected to match the desired integration, as long as it is able to interface with the driver via I2C and dedicated interrupt lines. This allows for interfaces based entirely on tactile buttons and 8-segment displays, based entirely on a touch screen, or on a combination of these.

Separate github projects will be created to track respective implementations.

## Hardware
This project is based on the STM32F103, and PCB designs on https://easyeda.com/ will eventually be linked and included.

### Encoder
In order to track the position of the spindle, a rotary encoder is used. 

### Motor
The driver outputs step/dir signals suitable to drive standard steppers.

## Inspiration
Loosely based on the core implementation from https://github.com/prototypicall/Didge

