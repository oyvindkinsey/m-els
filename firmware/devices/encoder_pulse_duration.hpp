
#include <Chip/STM32F103xx.hpp>
#include <Register/Register.hpp>

#include "../constants.hpp"
#include "../interrupts.hpp"

namespace devices {

    // todo: move to encoder.hpp
    struct encoder_pulse_duration {
        static constexpr uint16_t Prescaler = 4; // Period of a single channel contains 4 encoder changes
        volatile static inline uint16_t last_full_period = 0;

        using pin_ch2 = constants::pins::tim2_ch2;

        static void init() {
            using namespace Kvasir;
            // Pin is input floating by default so no action necessary
            // Timer registers - Configure for "PWM Input Mode"
            apply(write(Tim2Psc::psc, Prescaler - 1),
                write(Tim2Ccmr1Input::cc1s, 0b10),                               // Source input 2
                write(Tim2Ccmr1Input::cc2s, 0b01),                               // Source input 2
                set(Tim2Ccer::cc2p),                                             // invert polarity
                write(Tim2Smcr::ts, 0b110),                                      // trigger on filtered input 2
                write(Tim2Smcr::sms, 0b100),                                     // Slave mode: reset
                set(Tim2Dier::cc2de),                                            // enable DMA for IC2
                write(Tim2Ccr3::ccr3, std::numeric_limits<uint16_t>::max() - 1), // Timeout
                set(Tim2Dier::cc3ie)                                             // enable compare 3 interrupt
            );
            // DMA
            apply(write(Dma1Cpar7::pa, Tim2Ccr2::Addr::value), // Acc. to docs. Ch1 should have
                // the period value but Ch2 does
                write(Dma1Cmar7::ma, reinterpret_cast<unsigned>(std::addressof(last_full_period))),
                write(Dma1Cndtr7::ndt, 1),
                write(Dma1Ccr7::msize, 0b01), // 16 bits
                write(Dma1Ccr7::psize, 0b01),
                set(Dma1Ccr7::circ) // circular mode -> continuous
            );
            apply(set(Dma1Ccr7::en)); // enable DMA channel

            interrupts::enable<IRQ::tim2_irqn>();

            apply(set(Tim2Ccer::cc1e), set(Tim2Ccer::cc2e), // enable captures
                set(Tim2Cr1::cen));                       // enable timer
        }

        static inline void process_interrupt() {
            last_full_period = 0; // time out - too slow
            apply(clear(Kvasir::Tim2Sr::cc3if));
        }

        static inline uint16_t last_duration() {
            return last_full_period;
        }
    };
}