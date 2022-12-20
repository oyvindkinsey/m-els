
#include <Chip/STM32F103xx.hpp>
#include <Register/Register.hpp>
#include <Register/Utility.hpp>

#include "../constants.hpp"

namespace devices
{

    struct step_gen
    {
        static constexpr uint64_t ClockFreq = constants::CPU_Clock_Freq_Hz;
        static constexpr uint8_t ClockDiv = 2;

        static constexpr unsigned int min_count = constants::min_timer_capture_count; // required by timer

        using step_pin = constants::pins::step_pin;
        using dir_pin = constants::pins::dir_pin;

        struct start_stop
        {
            volatile uint16_t cnt_start{}, cnt_stop{};
        };

        struct State
        {
            start_stop counts_reverse{}, counts_delayed{};
            volatile uint16_t counts_step = 0; // step pulse duration in #timer clock cycles
            volatile bool delayed_pulse = false;
            volatile bool direction = false;          // true -> reverse direction
            volatile bool direction_polarity = false; // not inverted
        };

        static State state;

        static void init()
        {
            using namespace Kvasir;
            // Port
            apply(write(step_pin::cr::mode, gpio::PinMode::Output_2Mhz),
                  write(step_pin::cr::cnf, gpio::PinConfig::Output_alternate_push_pull),
                  write(dir_pin::cr::mode, gpio::PinMode::Output_2Mhz),
                  write(dir_pin::cr::cnf, gpio::PinConfig::Output_push_pull));
            // Timer
            apply(set(Tim3Cr1::opm),
                  write(Tim3Psc::psc, ClockDiv - 1),
                  write(Tim3Ccmr2Output::oc3m, 0b111), // PWM mode 2
                  write(Tim3Smcr::sms, 0b110),         // Trigger mode
                  write(Tim3Smcr::ts, 0),              // ITR0 - tim1
                  set(Tim3Ccer::cc3e),
                  clear(Tim3Sr::uif),
                  set(Tim3Dier::uie) // enable update interrupt
            );
            interrupts::enable<IRQ::tim3_irqn>();
        }

        static void configure(unsigned int dir_setup_ns, unsigned int step_pulse_ns,
                              bool invert_step, bool invert_dir)
        {
            apply(write(Kvasir::Tim3Ccer::cc3p, invert_step));
            state.direction_polarity = invert_dir;
            apply(write(dir_pin::odr, state.direction ^ state.direction_polarity));

            constexpr uint64_t TimerFreq = ClockFreq / ClockDiv;
            constexpr uint64_t nanosec = constants::onesec_in_ns.count();
            // TODO: implement a range checked version
            uint16_t cnt_setup_delay = std::max(min_count,
                                                1u + static_cast<unsigned int>(dir_setup_ns * TimerFreq / nanosec));
            uint16_t cnt_step = 1u + static_cast<unsigned int>(step_pulse_ns * TimerFreq / nanosec);
            state.counts_reverse = {cnt_setup_delay,
                                    static_cast<uint16_t>(cnt_setup_delay + cnt_step)};
            state.counts_step = cnt_step;
            setup_next_pulse();
        }

        static inline bool get_direction()
        {
            return state.direction;
        }

        static void change_direction(bool new_dir)
        {
            state.direction = new_dir;
            using namespace Kvasir;
            apply(write(dir_pin::odr, new_dir ^ state.direction_polarity));
            apply(clear(Tim3Ccmr2Output::oc3fe),
                  write(Tim3Ccr3::ccr3, state.counts_reverse.cnt_start),
                  write(Tim3Arr::arr, state.counts_reverse.cnt_stop));
        }

        static void set_delay(unsigned delay_count)
        {
            delay_count = delay_count / ClockDiv;
            if (delay_count >= min_count)
            {
                auto end_delayed = state.counts_step + delay_count;
                if (end_delayed >= std::numeric_limits<uint16_t>::max())
                {
                    end_delayed = std::numeric_limits<uint16_t>::max() - 1; // clamp
                    state.counts_delayed = {
                        static_cast<uint16_t>(end_delayed - state.counts_step),
                        static_cast<uint16_t>(end_delayed)};
                }
                else
                {
                    state.counts_delayed = {
                        static_cast<uint16_t>(delay_count),
                        static_cast<uint16_t>(end_delayed)};
                }
                state.delayed_pulse = true;
            }
            else
            {
                state.delayed_pulse = false;
            }
        }

        static inline void process_interrupt()
        {
            apply(clear(Kvasir::Tim3Sr::uif));
            setup_next_pulse();
        }

    private:
        static void setup_next_pulse()
        {
            using namespace Kvasir;
            if (state.delayed_pulse)
            {
                apply(clear(Tim3Ccmr2Output::oc3fe),
                      write(Tim3Ccr3::ccr3, state.counts_delayed.cnt_start),
                      write(Tim3Arr::arr, state.counts_delayed.cnt_stop));
            }
            else
            {
                apply(set(Tim3Ccmr2Output::oc3fe), // enable fast enable
                      write(Tim3Ccr3::ccr3, 1),    // For some reason 0 does not work
                      write(Tim3Arr::arr, state.counts_step));
            }
        }
    };
}