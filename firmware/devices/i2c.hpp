
#include "stm32f103xb.h"
#include "../constants.hpp"

namespace devices {

    struct i2c {
    public:
        static void init() {
            AFIO->MAPR |= AFIO_MAPR_I2C1_REMAP;

            GPIOB->CRH &= ~(GPIO_CRH_CNF8_Msk | GPIO_CRH_MODE8_Msk); // reset
            GPIOB->CRH |= GPIO_CRH_CNF8_0 | GPIO_CRH_CNF8_1; // Alternate function output Open-drain
            GPIOB->CRH |= GPIO_CRH_MODE8_0 | GPIO_CRH_MODE8_1; // Output mode, max speed 50MHz

            GPIOB->CRH &= ~(GPIO_CRH_CNF9_Msk | GPIO_CRH_MODE9_Msk); // reset
            GPIOB->CRH |= GPIO_CRH_CNF9_0 | GPIO_CRH_CNF9_1; // Alternate function output Open-drain
            GPIOB->CRH |= GPIO_CRH_MODE9_0 | GPIO_CRH_MODE9_1; // Output mode, max speed 50MHz

            // Configure I2C
            I2C1->CR1 &= ~I2C_CR1_PE; // disable
            I2C1->CR1 |= I2C_CR1_SWRST; // reset
            I2C1->CR1 &= ~I2C_CR1_SWRST;

            I2C1->CR2 |= 0x24;  //36mhz freq[5:0]

            I2C1->OAR1 |= 0x1 << 1; // set address to 0x55
            I2C1->CR1 |= I2C_CR1_PE; // enable

            // this must be set *after* PE
            I2C1->CR1 |= I2C_CR1_ACK;
        }
    };
}