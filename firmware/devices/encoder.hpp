#include "stm32f103xb.h"

namespace devices {

    struct encoder {
        using CounterValue = uint16_t;

        static void init() {
            // pins
            GPIOA->CRH &= ~(GPIO_CRH_CNF8_Msk | GPIO_CRH_MODE8_Msk); // clear the default bits
            GPIOA->CRH |= GPIO_CRH_CNF8_1; // input pull-down/pull-up
            GPIOA->BSRR |= GPIO_BSRR_BS8; // pull high

            GPIOA->CRH &= ~(GPIO_CRH_CNF9_Msk | GPIO_CRH_MODE9_Msk); // clear the default bits
            GPIOA->CRH |= GPIO_CRH_CNF9_1; // input pull-down/pull-up
            GPIOA->BSRR |= GPIO_BSRR_BS9; // pull high

            GPIOA->CRH &= ~(GPIO_CRH_CNF10_Msk | GPIO_CRH_MODE10_Msk); // general purpose output push-pull
            GPIOA->CRH |= GPIO_CRH_CNF10_1; // Alternate function output Push-pull
            GPIOA->CRH |= GPIO_CRH_MODE10_1; // Output mode, max speed 2MHz

            TIM1->CCMR1 |= TIM_CCMR1_CC1S_0; // CC1 channel is configured as input, IC1 is mapped on TI1
            TIM1->CCMR1 |= TIM_CCMR1_IC1F_0 | TIM_CCMR1_IC1F_1; // filter: n = 8
            TIM1->CCMR1 |= TIM_CCMR1_CC2S_0; // CC2 channel is configured asinput, IC2 is mapped on TI2
            TIM1->CCMR1 |= TIM_CCMR1_IC2F_0 | TIM_CCMR1_IC1F_1; // filter: n = 8
            TIM1->CCMR2 |= TIM_CCMR2_OC3M_0; // Set on match
            TIM1->SMCR |= TIM_SMCR_SMS_0 | TIM_SMCR_SMS_1; // both input active on both rising and falling edges
            TIM1->CR2 |= TIM_CR2_MMS_1 | TIM_CR2_MMS_2; // OC3REF signal is used as trigger output (TRGO) 
            TIM1->CCER |= TIM_CCER_CC3E; // Capture/Compare 3 ouput enable 
            TIM1->BDTR |= TIM_BDTR_MOE; // OC and OCN outputs are enabled if the respective bits is set in CCER 

            TIM1->CR1 |= TIM_CR1_CEN; // Counter enabled

            setup_cc_interrupt();
        }

        static void setup_cc_interrupt() {
            TIM1->DIER |= TIM_DIER_CC3IE; // CC3 interrupt enabled
            TIM1->DIER |= TIM_DIER_CC4IE; // CC4 interrupt enabled
            clear_cc_interrupt();
            NVIC_EnableIRQ(TIM1_CC_IRQn);
        }

        static inline bool is_cc_fwd_interrupt() {
            return TIM1->SR & TIM_SR_CC3IF_Msk;
        }

        static inline void clear_cc_interrupt() {
            TIM1->SR &= ~(TIM_SR_CC3IF_Msk | TIM_SR_CC4IF_Msk);
        }

        static inline void update_channels(CounterValue next, CounterValue prev) {
            TIM1->CCR3 = next;
            TIM1->CCR4 = prev;
        }

        static void inline trigger_clear() {
            TIM1->CCMR2 &= ~TIM_CCMR2_OC3M_Msk;
            TIM1->CCMR2 |= TIM_CCMR2_OC3M_2; // force oc3ref low
        }

        static void inline trigger_restore() {
            TIM1->CCMR2 |= TIM_CCMR2_OC3M_0; // Set on match
        }

        static void inline trigger_manual_pulse() {
            TIM1->CCMR2 &= ~TIM_CCMR2_OC3M_Msk;
            TIM1->CCMR2 |= TIM_CCMR2_OC3M_0 | TIM_CCMR2_OC3M_2; // Force oc3ref high
            trigger_clear();
            trigger_restore();
        }

        static inline CounterValue get_count() {
            return TIM1->CNT;
        }
    };

}