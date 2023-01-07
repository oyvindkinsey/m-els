
#include "stm32f103xb.h"
#include "../constants.hpp"

namespace devices {

    typedef struct {
        char rpm_l;
        char rpm_h;
        char gear_num;
        char gear_denom;
        char r5;
        char r6;
        char r8;
    } register_t;

    struct i2c {

        volatile static inline char dma_buffer[16];

    public:
        volatile static inline register_t reg;
        static void init() {
            RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;

            GPIOB->CRH &= ~(GPIO_CRH_CNF10_Msk | GPIO_CRH_MODE10_Msk); // reset
            GPIOB->CRH |= GPIO_CRH_CNF10_0 | GPIO_CRH_CNF10_1; // Alternate function output Open-drain
            GPIOB->CRH |= GPIO_CRH_MODE10_0 | GPIO_CRH_MODE10_1; // Output mode, max speed 50MHz

            GPIOB->CRH &= ~(GPIO_CRH_CNF11_Msk | GPIO_CRH_MODE11_Msk); // reset
            GPIOB->CRH |= GPIO_CRH_CNF11_0 | GPIO_CRH_CNF11_1; // Alternate function output Open-drain
            GPIOB->CRH |= GPIO_CRH_MODE11_0 | GPIO_CRH_MODE11_1; // Output mode, max speed 50MHz

            // Set up DMA for RX
            DMA1_Channel5->CCR &= ~(DMA_CCR_EN | DMA_CCR_PINC_Msk | DMA_CCR_MSIZE_Msk | DMA_CCR_PSIZE_Msk | DMA_CCR_DIR_Msk);
            DMA1_Channel5->CPAR = (uint32_t)&I2C2->DR; // source
            DMA1_Channel5->CCR |= DMA_CCR_MINC; // memory increment mode
            DMA1_Channel5->CCR |= DMA_CCR_CIRC; // circular mode
            DMA1_Channel5->CCR |= DMA_CCR_TCIE; // transfer complete interrupt enable

            // Set up DMA for TX - CMAR and CNDTR are set later
            DMA1_Channel4->CCR &= ~(DMA_CCR_EN | DMA_CCR_PINC_Msk | DMA_CCR_MSIZE_Msk | DMA_CCR_PSIZE_Msk | DMA_CCR_CIRC_Msk);
            DMA1_Channel4->CPAR = (uint32_t)&I2C2->DR; // dest
            DMA1_Channel4->CCR |= DMA_CCR_MINC; // memory increment mode
            DMA1_Channel4->CCR |= DMA_CCR_DIR; // from memory to peripheral

            // Configure I2C
            I2C2->CR1 &= ~I2C_CR1_PE; // disable
            I2C2->CR1 |= I2C_CR1_SWRST; // reset
            I2C2->CR1 &= ~I2C_CR1_SWRST;
            I2C2->CR2 |= 0x24;  //36mhz freq[5:0]
            I2C2->OAR1 |= 0x1 << 1; // set address to 0x55
            I2C2->CR2 |= I2C_CR2_ITEVTEN;
            I2C2->CR1 |= I2C_CR1_PE; // enable

            // this must be set *after* PE
            I2C2->CR1 |= I2C_CR1_ACK;

            NVIC_SetPriority(DMA1_Channel5_IRQn, 1);
            NVIC_SetPriority(I2C2_EV_IRQn, 2);

            NVIC_EnableIRQ(DMA1_Channel5_IRQn);
            NVIC_EnableIRQ(I2C2_EV_IRQn);
        }

        static void set_rpm(uint16_t rpm) {
            reg.rpm_l = (uint8_t)(0x00FF & rpm);
            reg.rpm_h = (uint8_t)(rpm >> 8);
        }

        // RX
        static void DMA1_Channel5_IRQHandler() {
            if (DMA1->ISR & DMA_ISR_TCIF5) {
                rx_complete();
            }
        }

        static void rx_complete() {
            DMA1->IFCR |= DMA_IFCR_CTCIF5;
            DMA1->IFCR |= DMA_IFCR_CTCIF4;
            DMA1_Channel5->CCR &= ~DMA_CCR_EN;
            DMA1_Channel4->CCR &= ~DMA_CCR_EN;
            I2C2->CR2 &= ~I2C_CR2_DMAEN;
            auto len = 16 - DMA1_Channel5->CNDTR;
            if (len > 1) {
                uint32_t base = (uint32_t)&reg;
                uint8_t offset = dma_buffer[0];
                uint32_t dest = base + offset;
                auto incr = sizeof(char);
                for (auto i = 0; i < len; i++) {
                    *((uint32_t*)(uintptr_t)dest) = dma_buffer[i + 1];
                    dest += incr;
                }
            }
        }

        static void rx_start() {
            DMA1_Channel5->CCR &= ~DMA_CCR_EN;
            DMA1_Channel5->CNDTR = 16; // length of data to expect
            DMA1_Channel5->CMAR = (uint32_t)&dma_buffer; // dest
            DMA1_Channel5->CCR |= DMA_CCR_EN;
            I2C2->CR2 |= I2C_CR2_DMAEN;
        }

        static void tx_start() {
            uint32_t base = (uint32_t)&reg;
            uint8_t offset = dma_buffer[0];
            DMA1_Channel4->CCR &= ~DMA_CCR_EN;
            DMA1_Channel4->CMAR = base + offset;
            DMA1_Channel4->CNDTR = 16;
            DMA1_Channel4->CCR |= DMA_CCR_EN;
            I2C2->CR2 |= I2C_CR2_DMAEN;
        }

        static void I2C2_EV_IRQHandler() {
            if (I2C2->SR1 & I2C_SR1_ADDR) { // address matched
                if (I2C2->SR2 & I2C_SR2_TRA) {
                    tx_start();
                } else {
                    rx_start();
                }
            }

            if (I2C2->SR1 & I2C_SR1_STOPF) {
                // clear the bit
                I2C2->CR1 |= I2C_CR1_ACK;

                if (!(I2C2->SR2 & I2C_SR2_TRA)) {
                    rx_complete();
                }
            }
        }
    };
}