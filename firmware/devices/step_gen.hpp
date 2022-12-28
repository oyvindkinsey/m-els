#include "stm32f103xb.h"
#include "../constants.hpp"

namespace devices {

    struct step_gen {
        static constexpr uint64_t ClockFreq = constants::CPU_Clock_Freq_Hz;
        static constexpr uint8_t ClockDiv = 2;

        static constexpr unsigned int min_count = constants::min_timer_capture_count; // required by timer

        struct start_stop {
            volatile uint16_t cnt_start{}, cnt_stop{};
        };

        struct State {
            start_stop counts_reverse{}, counts_delayed{};
            volatile uint16_t counts_step = 0; // step pulse duration in #timer clock cycles
            volatile bool delayed_pulse = false;
            volatile bool direction = false;          // true -> reverse direction
            volatile bool direction_polarity = false; // not inverted
        };

        static State state;

        static void init() {
            // step
            GPIOB->CRL &= ~(GPIO_CRL_CNF0_Msk | GPIO_CRL_MODE0_Msk); // reset
            GPIOB->CRL |= GPIO_CRL_CNF0_1; // Alternate function output Push-pull
            GPIOB->CRL |= GPIO_CRL_MODE0_1; // Output mode, max speed 2MHz

            // dir
            GPIOB->CRL &= ~(GPIO_CRL_CNF1_Msk | GPIO_CRL_MODE1_Msk); // reset
            GPIOB->CRL |= GPIO_CRL_MODE1_1; // Output mode, max speed 2MHz

            // Timer
            TIM3->CR1 |= TIM_CR1_OPM; // one pulse mode
            TIM3->PSC = ClockDiv - 1; // set the prescaler
            TIM3->CCMR2 &= ~TIM_CCMR2_OC3M_Msk;
            TIM3->CCMR2 |= TIM_CCMR2_OC3M_0 | TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2; // PWM mode 2
            TIM3->SMCR &= ~TIM_SMCR_SMS_Msk;
            TIM3->SMCR |= TIM_SMCR_SMS_1 | TIM_SMCR_SMS_2; // Trigger mode
            TIM3->SMCR &= ~TIM_SMCR_TS_Msk; // Internal Trigger 0 (ITR0), shared with tim1
            TIM3->CCER |= TIM_CCER_CC3E; // Capture/Compare 3 output enable
            TIM3->SR &= ~TIM_SR_UIF_Msk; // Clear the update interrupt flag
            TIM3->DIER |= TIM_DIER_UIE; // Update interrupt enabled

            NVIC_EnableIRQ(TIM3_IRQn);
        }

        static void configure(unsigned int dir_setup_ns, unsigned int step_pulse_ns,
            bool invert_step, bool invert_dir) {
            if (invert_step) {
                TIM3->CCER |= TIM_CCER_CC3P;
            } else {

                TIM3->CCER &= ~TIM_CCER_CC3P;
            }

            state.direction_polarity = invert_dir;
            if (state.direction ^ state.direction_polarity) {
                GPIOB->ODR |= GPIO_ODR_ODR1;
            } else {
                GPIOB->ODR &= ~GPIO_ODR_ODR1;
            }

            constexpr uint64_t TimerFreq = ClockFreq / ClockDiv;
            constexpr uint64_t nanosec = constants::onesec_in_ns.count();
            // TODO: implement a range checked version
            uint16_t cnt_setup_delay = std::max(min_count,
                1u + static_cast<unsigned int>(dir_setup_ns * TimerFreq / nanosec));
            uint16_t cnt_step = 1u + static_cast<unsigned int>(step_pulse_ns * TimerFreq / nanosec);
            state.counts_reverse = { cnt_setup_delay,
                                    static_cast<uint16_t>(cnt_setup_delay + cnt_step) };
            state.counts_step = cnt_step;
            setup_next_pulse();
        }

        static inline bool get_direction() {
            return state.direction;
        }

        static void change_direction(bool new_dir) {
            state.direction = new_dir;
            if (new_dir ^ state.direction_polarity) {
                GPIOB->ODR |= GPIO_ODR_ODR1;
            } else {
                GPIOB->ODR &= ~GPIO_ODR_ODR1;
            }


            TIM3->CCMR2 &= ~TIM_CCMR2_OC3FE_Msk; // output compare 3 fast disable
            TIM3->CCR3 = state.counts_reverse.cnt_start; // load new capture/compare value
            TIM3->ARR = state.counts_reverse.cnt_stop; //  load the new reload value
        }

        static void set_delay(unsigned delay_count) {
            delay_count = delay_count / ClockDiv;
            if (delay_count >= min_count) {
                auto end_delayed = state.counts_step + delay_count;
                if (end_delayed >= std::numeric_limits<uint16_t>::max()) {
                    end_delayed = std::numeric_limits<uint16_t>::max() - 1; // clamp
                    state.counts_delayed = {
                        static_cast<uint16_t>(end_delayed - state.counts_step),
                        static_cast<uint16_t>(end_delayed) };
                } else {
                    state.counts_delayed = {
                        static_cast<uint16_t>(delay_count),
                        static_cast<uint16_t>(end_delayed) };
                }
                state.delayed_pulse = true;
            } else {
                state.delayed_pulse = false;
            }
        }

        static inline void process_interrupt() {
            TIM3->SR &= ~TIM_SR_UIF_Msk;
            setup_next_pulse();
        }

    private:
        static void setup_next_pulse() {
            if (state.delayed_pulse) {
                TIM3->CCMR2 &= ~TIM_CCMR2_OC3FE_Msk; // output compare 3 fast disable
                TIM3->CCR3 = state.counts_delayed.cnt_start; // load new capture/compare value
                TIM3->ARR = state.counts_delayed.cnt_stop; //  load the new reload value
            } else {
                TIM3->CCMR2 |= TIM_CCMR2_OC3FE; // enable fast enable
                TIM3->CCR3 = 1; // For some reason 0 does not work
                TIM3->ARR = state.counts_step;
            }
        }
    };
}