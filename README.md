# Modular Electronic Leadscrew
m-els is a project for an electronic leadscrew that is focused on implementing a highly performant and flexible electronic leadscrew.
Its main design is a driver based on STM32, which implements the core translation from encoder input to stepper output, and which is controlled via I2C and select interrupts.

## Components
### Driver
This component is optimized for performance, and uses low level port manipulation, interrupts, and DMA for all data transfer. 
Since this is free of having to handle input and displays, its resources can be used entirely on implementing core functionality.

#### Interface
The driver acts as an I2C slave, and has its 'gearing' set by writing to exposed registers. The driver also expose interrupt lines, where the interrupt type and related information can be read from respective registers, and subsequently cleared.

##### Registers
The following struct is exposed for read/write:
```
    typedef struct {
        char rpm_l;
        char rpm_h;
        char flags; // [stepper_en, rpm_en, reserved, reserved,reserved ,reserved, reserved, reserved, reserved]
        char gear_num;
        char gear_denom;
    } register_t;
```

* rpm_l (r): The lower 32 bits of a 64 bit int representing the current RPM.
* rpm_h (r): The higher 32 bits of a 64 bit int representing the current RPM.
* flags (rw): Bitmap used to read and change status. Only some bits are writable.
* gear_num (rw): The numerator for the gearing, representing the 'teeth on the driving gear' in the virtual gearing.
* gear_denom (rw): The denomenator for the gearing, representing the 'teeth on the driven gear' in the virtual gearing.

##### Interrupts
An interrupt will be raised whenever the flags change, such as during a fault situation, or when I/O triggers a state change.

##### I/O
In addition to I2C, certain operations can be triggered via external interrupt lines.
* E-Stop: Halts the driver immediately. Implementation TBD.
* Start: Starts the leadscrew. Implementation TBD.
* Stop: Stops the leadscrew. Implementation TBD.
* Jog/Move Left: If in jog mode, drives the carriage to the left. If in cycle mode, moves to the left-most limit. Implementation TBD.
* Jog/Move Right: If in jog mode, drives the carriage to the right. If in cycle mode, moves to the right-most limit. Implementation TBD.

### Planned Features
* Set left and right limit
* Jog left and right
* Move to left and right limit

### User Interface
A user interface can be selected to match the desired integration, as long as it is able to interface with the driver via I2C and dedicated interrupt lines. This allows for interfaces based entirely on tactile buttons and 8-segment displays, based entirely on a touch screen, or on a combination of these.

Separate github projects will be created to track respective implementations.

## Hardware
This project is based on the STM32F103, and PCB designs on https://easyeda.com/ will eventually be linked and included.

### Encoder
In order to track the position of the spindle, a rotary encoder is used. The project expects this to have a resolution of 2400.
This is configurable in [constants.hpp](firmware/constants.hpp).

### Motor
The driver outputs step/dir signals suitable to drive standard steppers. The project expects this to have 200 steps per resolution, with a micro stepping of 10.
Each step pulse is set to last for 2500ns, and changes to direction is held for 5000ns before the next step pulse is sent.
This is configurable in [constants.hpp](firmware/constants.hpp).

## Inspiration
Loosely based on the core implementation from https://github.com/prototypicall/Didge 

