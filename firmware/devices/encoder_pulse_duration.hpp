#include "stm32f103xb.h"

namespace devices {

    // todo: move to encoder.hpp
    struct encoder_pulse_duration {
        static constexpr uint16_t Prescaler = 4; // Period of a single channel contains 4 encoder changes
        volatile static inline uint16_t last_full_period = 0;


        static void init() {
            // set up the timer
            TIM2->PSC = Prescaler - 1;
            TIM2->CCMR1 |= TIM_CCMR1_CC1S_1; // CC1 channel is configured as input, IC1 is mapped on TI2
            TIM2->CCMR1 |= TIM_CCMR1_CC2S_0; // CC2 channel is configured as input, IC2 is mapped on TI2
            TIM2->CCER |= TIM_CCER_CC2P; // Active on falling edge
            TIM2->SMCR |= TIM_SMCR_TS_1 | TIM_SMCR_TS_2; // Trigger on filtered input 2
            TIM2->SMCR |= TIM_SMCR_SMS_2; // Reset Mode
            TIM2->DIER |= TIM_DIER_CC2DE; // CC2 DMA request enabled
            TIM2->CCR3 = std::numeric_limits<uint16_t>::max() - 1; // Capture/Compare value
            TIM2->DIER |= TIM_DIER_CC3IE; // CC3 interrupt enabled

            // set up DMA to capture the lenght of the last full period
            DMA1_Channel7->CPAR = reinterpret_cast<unsigned>(std::addressof(TIM2->CCR2)); // Set the peripheral address register to that of TIM2's Capture/Compare register 2  
            DMA1_Channel7->CMAR = reinterpret_cast<unsigned>(std::addressof(last_full_period));
            DMA1_Channel7->CNDTR = 1; // the number of data to be transferred
            DMA1_Channel7->CCR &= ~(DMA_CCR_MSIZE |
                DMA_CCR_PSIZE |
                DMA_CCR_EN);
            DMA1_Channel7->CCR |= (0x1 << DMA_CCR_MSIZE_Pos) // 16 bits 
                | (0x1 << DMA_CCR_PSIZE_Pos) // 16 bits
                | DMA_CCR_CIRC; // circular mode -> continuous

            // start everything
            DMA1_Channel7->CCR |= DMA_CCR_EN; // enable DMA
            NVIC_EnableIRQ(TIM2_IRQn); // enable the interrupt
            TIM2->CCER |= TIM_CCER_CC1E; // enable capture
            TIM2->CR1 |= TIM_CR1_CEN; // enable timer
        }

        static inline void process_interrupt() {
            last_full_period = 0; // time out - too slow
            TIM2->SR &= ~TIM_SR_CC3IF_Msk;
        }

        static inline uint16_t last_duration() {
            return last_full_period;
        }
    };
}