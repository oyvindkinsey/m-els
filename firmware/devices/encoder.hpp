
#include <Chip/STM32F103xx.hpp>
#include <Register/Register.hpp>

#include "../constants.hpp"

namespace devices {

    struct encoder {
        using pin_A = constants::pins::enc_A;
        using pin_B = constants::pins::enc_B;
        using pin_Ch3 = constants::pins::tim1_ch3;

        // TODO: configurable polarity selection and input filter
        static constexpr auto input_filter = 0b0011; // 0b0011 for N = 8 samples

        using CounterValue = uint16_t;

        static void init() {
            // Pins
            using namespace Kvasir;
            apply(write(pin_A::cr::cnf, gpio::PinConfig::Input_pullup_pulldown),
                set(pin_A::bsrr),
                write(pin_B::cr::cnf, gpio::PinConfig::Input_pullup_pulldown),
                set(pin_B::bsrr));
            apply(write(pin_Ch3::cr::mode, gpio::PinMode::Output_2Mhz),
                write(pin_Ch3::cr::cnf, gpio::PinConfig::Output_alternate_push_pull));
            // Timer
            apply(write(Tim1Ccmr1Input::cc1s, 0b01),
                write(Tim1Ccmr1Input::ic1f, input_filter),
                write(Tim1Ccmr1Input::cc2s, 0b01),
                write(Tim1Ccmr1Input::ic2f, input_filter),
                write(Tim1Ccmr2Output::oc3m, 0b001), // Set on match
                write(Tim1Smcr::sms, 0b011),
                write(Tim1Cr2::mms, 0b110), // oc3ref is TRGO
                set(Tim1Ccer::cc3e),        // enable ch3 output
                set(Tim1Bdtr::moe)          // enable outputs
            );
            apply(set(Tim1Cr1::cen));

            setup_cc_interrupt();
        }

        static void setup_cc_interrupt() {
            using namespace Kvasir;
            apply(set(Tim1Dier::cc3ie), set(Tim1Dier::cc4ie));
            clear_cc_interrupt();
            interrupts::enable<IRQ::tim1_cc_irqn>();
        }

        static inline bool is_cc_fwd_interrupt() {
            using namespace Kvasir;
            return apply(read(Tim1Sr::cc3if));
        }

        static inline void clear_cc_interrupt() {
            using namespace Kvasir;
            apply(clear(Tim1Sr::cc3if),
                clear(Tim1Sr::cc4if));
        }

        static inline void update_channels(CounterValue next, CounterValue prev) {
            using namespace Kvasir;
            apply(write(Tim1Ccr3::ccr3, next),
                write(Tim1Ccr4::ccr4, prev));
        }

        static void inline trigger_clear() {
            apply(write(Kvasir::Tim1Ccmr2Output::oc3m, 0b100)); // force oc3ref low
        }

        static void inline trigger_restore() {
            apply(write(Kvasir::Tim1Ccmr2Output::oc3m, 0b001)); // set oc3ref on match
        }

        static void inline trigger_manual_pulse() {
            apply(write(Kvasir::Tim1Ccmr2Output::oc3m, 0b101)); // Force oc3ref high
            trigger_clear();
            trigger_restore();
        }

        static inline CounterValue get_count() {
            return apply(read(Kvasir::Tim1Cnt::cnt));
        }
    };

}