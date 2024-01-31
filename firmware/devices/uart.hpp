#pragma once

#include "stm32f103xb.h"
#include "../constants.hpp"

namespace devices {
    constexpr uint64_t BaudRate = 115200;

    struct uart {

        static void write(int ch) {
            /*Make sure the transmit data register is empty*/
            while (!(USART1->SR & USART_SR_TXE)) {}

            /*Write to transmit data register*/
            USART1->DR = (ch & 0xFF);
        }

        static void  write(char* data, int size) {
            int count = size;
            while (count--) {
                while (!(USART1->SR & USART_SR_TXE)) {};
                USART1->DR = *data++;
            }
        }

        static void init() {
            RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

            // Connect USART1 to alternate pins 6 and 7
            AFIO->MAPR |= AFIO_MAPR_USART1_REMAP;

            // Set up tx pin
            GPIOB->CRL &= ~(GPIO_CRL_CNF6_Msk | GPIO_CRL_MODE6_Msk); // reset
            GPIOB->CRL |= GPIO_CRL_CNF6_1; // Alternate function output Push-pull
            GPIOB->CRL |= GPIO_CRL_MODE6_1; // Output mode, max speed 2MHz

            // Set up rx pin
            GPIOB->CRL &= ~(GPIO_CRL_CNF7_Msk | GPIO_CRL_MODE7_Msk); // reset
            GPIOB->CRL |= GPIO_CRL_CNF7_0; // Floating input

            // Configure USART1
            USART1->BRR = constants::CPU_Clock_Freq_Hz / BaudRate; // baud rate register
            USART1->CR1 |= USART_CR1_TE; // Transmitter enable

            // Enable USART1
            USART1->CR1 |= USART_CR1_UE; // USART enable
        }
    };
}
