## Modular Electronic Leadscrew
m-els is a project for an electronic leadscrew that is focused on implementing a highly performant and flexible electronic leadscrew.
It's main design is a driver based on STM32, which implements the core translation from encoder input to stepper output, and which is controlled via I2C.

### Driver
This component is optimized for performance, and uses low level port manipulation, interrupts, and DMA for all data transfer. 

### Interface
The driver acts as an I2C slave, and has its 'gearing' set by writing to exposed registers. The driver also expose interrupt lines, where the interrupt type and related information can be read from respective registers, and subsequently cleared.

## Inspiration
Loosely based on the core implementation from https://github.com/prototypicall/Didge 

