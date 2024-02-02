#pragma once
#include "stm32f103xb.h"
#include "../constants.hpp"
#include "rpm.hpp"
#include "encoder.hpp"

namespace devices {

    using gearing_ratio_t = std::pair<uint16_t, uint16_t>;

    constexpr char version{ 1 };

#pragma pack(1)
    typedef struct {
        uint8_t version;
    } reg_info_t;

    // todo: move most constants into this
#pragma pack(1)
    typedef struct {

        // resolution of encoder
        uint16_t encoder_resolution;

        // stepper resolution
        uint16_t stepper_resolution;
        // stepper pulse width
        uint16_t stepper_pulse_length_ns;
        // dwell time when changing direction
        uint16_t stepper_change_dwell_ns;

        // [invert_step_pin, invert_dir_pin, ...]
        char stepper_flags;

    } reg_configuration_t;

#pragma pack(1)
    typedef struct {
        char mode;
        uint8_t gear_num;
        uint8_t gear_denom;
    } reg_settings_t;

#pragma pack(1)
    typedef struct {
        uint16_t rpm;
        uint16_t pos;
    } reg_state_t;

#pragma pack(0)

    struct i2c {

    private:
        volatile static inline char dma_buffer[16];

        static uint32_t get_address(uint8_t offset) {
            if (offset >= 50) {
                return (uint32_t)&reg_state + offset - 50;
            }
            if (offset >= 30) {
                return (uint32_t)&reg_settings + offset - 30;
            }
            if (offset >= 10) {
                return (uint32_t)&reg_configuration + offset - 10;
            }
            return (uint32_t)&reg_info + offset;
        }

        static void rx_complete() {
            DMA1->IFCR |= DMA_IFCR_CTCIF5;
            DMA1->IFCR |= DMA_IFCR_CTCIF4;
            DMA1_Channel5->CCR &= ~DMA_CCR_EN;
            DMA1_Channel4->CCR &= ~DMA_CCR_EN;

            auto len = 16 - DMA1_Channel5->CNDTR;
            if (len == 1) {
                // prepare for a read
                reg_state.rpm = rpm_counter<>::get_rpm(reg_configuration.encoder_resolution);
                reg_state.pos = encoder::get_count();
            } else {
                // perform a write
                uint32_t dest = get_address(dma_buffer[0]);
                len--;
                auto incr = sizeof(char);
                for (auto i = 0; i < len; i++) {
                    *((char*)(uintptr_t)dest) = dma_buffer[i + 1];
                    dest += incr;
                }
            }
            dma_buffer[0] = 0x0;
        }

        static void rx_start() {
            DMA1_Channel5->CCR &= ~DMA_CCR_EN;
            DMA1_Channel4->CCR &= ~DMA_CCR_EN;
            DMA1_Channel5->CNDTR = 16; // length of data to expect
            DMA1_Channel5->CMAR = (uint32_t)&dma_buffer; // dest
            DMA1_Channel5->CCR |= DMA_CCR_EN;
        }

        static void tx_start() {
            DMA1_Channel5->CCR &= ~DMA_CCR_EN;
            DMA1_Channel4->CCR &= ~DMA_CCR_EN;
            DMA1_Channel4->CMAR = get_address(dma_buffer[0]);
            DMA1_Channel4->CNDTR = 16;
            DMA1_Channel4->CCR |= DMA_CCR_EN;
        }

    public:
        // offset 0 - reserve 10
        volatile static inline reg_info_t reg_info = {
            .version = version
        };

        // offset 10 - reserve 20
        volatile static inline reg_configuration_t reg_configuration = {
            .encoder_resolution = 2400u,
            .stepper_resolution = 200u * 10,
            .stepper_pulse_length_ns = 2500,
            .stepper_change_dwell_ns = 5000,
            .stepper_flags = 0x2
        };
        // offset 30 - reserve 20
        volatile static inline reg_settings_t reg_settings = {
            .mode = 0,
            .gear_num = 1,
            .gear_denom = 1
        };
        // offset 50 - reserve 20
        volatile static inline reg_state_t reg_state = {
            .rpm = 0,
            .pos = 0
        };

        static void init() {

            RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;

            GPIOB->CRH &= ~(GPIO_CRH_CNF10_Msk | GPIO_CRH_MODE10_Msk); // reset
            GPIOB->CRH |= GPIO_CRH_CNF10_0 | GPIO_CRH_CNF10_1; // Alternate function output Open-drain
            GPIOB->CRH |= GPIO_CRH_MODE10_0 | GPIO_CRH_MODE10_1; // Output mode, max speed 50MHz

            GPIOB->CRH &= ~(GPIO_CRH_CNF11_Msk | GPIO_CRH_MODE11_Msk); // reset
            GPIOB->CRH |= GPIO_CRH_CNF11_0 | GPIO_CRH_CNF11_1; // Alternate function output Open-drain
            GPIOB->CRH |= GPIO_CRH_MODE11_0 | GPIO_CRH_MODE11_1; // Output mode, max speed 50MHz

            // Set up DMA for RX
            DMA1_Channel5->CCR &= ~(DMA_CCR_EN | DMA_CCR_PINC_Msk | DMA_CCR_MSIZE_Msk | DMA_CCR_PSIZE_Msk | DMA_CCR_CIRC_Msk | DMA_CCR_DIR_Msk);
            DMA1_Channel5->CPAR = (uint32_t)&I2C2->DR; // source
            DMA1_Channel5->CCR |= DMA_CCR_MINC; // memory increment mode
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
            I2C2->OAR1 |= 0x1 << 1; // set address to 0x1
            I2C2->CR2 |= I2C_CR2_ITEVTEN;
            I2C2->CR2 |= I2C_CR2_DMAEN;
            I2C2->CR1 |= I2C_CR1_PE; // enable

            // this must be set *after* PE
            I2C2->CR1 |= I2C_CR1_ACK;

            NVIC_SetPriority(DMA1_Channel5_IRQn, 1);
            NVIC_SetPriority(I2C2_EV_IRQn, 2);

            NVIC_EnableIRQ(DMA1_Channel5_IRQn);
            NVIC_EnableIRQ(I2C2_EV_IRQn);
        }

        // RX
        static void DMA1_Channel5_IRQHandler() {
            if (DMA1->ISR & DMA_ISR_TCIF5) {
                rx_complete();
            }
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