#include "stm32f103xb.h"

namespace devices {
    struct debug {

        static void init() {
            GPIOC->CRH &= ~GPIO_CRH_CNF13_Msk | GPIO_CRH_MODE13_Msk; // general purpose output push-pull
            GPIOC->CRH |= GPIO_CRH_MODE13_1; // Output mode, max speed 2MHz
        }

        static void toggle_led() {
            GPIOC->ODR ^= GPIO_ODR_ODR13_Msk;
        }
    };
}