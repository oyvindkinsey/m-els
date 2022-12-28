
#include <Chip/STM32F103xx.hpp>
#include <Register/Register.hpp>

#include "../constants.hpp"

namespace devices {
    struct debug {

        static void init() {
            using namespace constants;
            using namespace Kvasir;

            apply(write(pins::led_pin::cr::mode, gpio::PinMode::Output_2Mhz));
        }

        static void toggle_led() {
            using namespace constants;

            bool led = apply(read(pins::led_pin::odr));
            apply(write(pins::led_pin::odr, !led));
        }
    };
}