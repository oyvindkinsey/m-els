
#include "stm32f103xb.h"
#include "../constants.hpp"

namespace devices {

    struct i2c {

        volatile static inline char dma_buff_in[4];
        volatile static inline bool send_stop;
        volatile static inline char registers[16];

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

            // wtf - why does this kill systick
            I2C1->CR2 |= I2C_CR2_ITEVTEN;

            // Set up DMA for RX
            DMA1_Channel7->CCR &= ~(DMA_CCR_EN | DMA_CCR_PINC_Msk | DMA_CCR_MSIZE_Msk | DMA_CCR_PSIZE_Msk | DMA_CCR_DIR_Msk);
            DMA1_Channel7->CPAR = (uint32_t)&I2C1->DR; // source
            DMA1_Channel7->CCR |= DMA_CCR_MINC; // memory increment mode
            DMA1_Channel7->CCR |= DMA_CCR_CIRC; // circular mode
            DMA1_Channel7->CCR |= DMA_CCR_TCIE; // transfer complete interrupt enable

            I2C1->CR1 |= I2C_CR1_PE; // enable
            // this must be set *after* PE
            I2C1->CR1 |= I2C_CR1_ACK;

            NVIC_EnableIRQ(DMA1_Channel7_IRQn);
            NVIC_SetPriority(DMA1_Channel7_IRQn, 1);
            NVIC_SetPriority(I2C1_EV_IRQn, 2);
            NVIC_EnableIRQ(I2C1_EV_IRQn);
        }

        static void set_rpm(uint16_t rpm) {
            // store in register 0 and 1

            registers[0] = rpm >> 8;
            registers[1] = 0x0F & rpm;

        }

        static void DMA1_Channel7_IRQHandler() {
            if (DMA1->ISR & DMA_ISR_TCIF7) {
                DMA1->IFCR |= DMA_IFCR_CTCIF7;
                DMA1_Channel7->CCR &= ~DMA_CCR_EN;
                I2C1->CR2 &= ~I2C_CR2_DMAEN;
            }
        }

        static void I2C1_EV_IRQHandler() {
            if (I2C1->SR1 & I2C_SR1_ADDR) { // address matched
                if (I2C1->SR2 & I2C_SR2_TRA) {
                    // transmitter
                    switch (dma_buff_in[0]) {
                    case 0x1:
                        I2C1->DR = registers[0];
                        break;
                    case 0x2:
                        I2C1->DR = registers[1];
                        break;
                    }
                    send_stop = true;
                } else {
                    // receiver
                    DMA1_Channel7->CCR &= ~DMA_CCR_EN;
                    DMA1_Channel7->CNDTR = 1; // length of data to expect
                    DMA1_Channel7->CMAR = (uint32_t)&dma_buff_in; // dest
                    DMA1_Channel7->CCR |= DMA_CCR_EN;
                    I2C1->CR2 |= I2C_CR2_DMAEN;
                }
            }

            if (I2C1->SR1 & I2C_SR1_STOPF) {
                // release the line if needed
                if (send_stop) {
                    I2C1->CR1 |= I2C_CR1_STOP;
                    send_stop = false;
                }
            }

        }
    };
}