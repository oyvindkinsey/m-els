#pragma once

#include <Chip/STM32F103xx.hpp>
#include <Register/Register.hpp>

namespace interrupts
{

    // Static (i.e. compile time) map of used IRQs and their values
    // if you are getting a compile time error, you might be missing an entry.
    constexpr uint8_t priorities(Kvasir::nvic::irq_number_t irq)
    {
        switch (irq)
        {
        case Kvasir::IRQ::tim2_irqn:
            return 1;
        case Kvasir::IRQ::tim1_cc_irqn:
            return 2;
        case Kvasir::IRQ::tim3_irqn:
            return 4;
        case Kvasir::IRQ::usart1_irqn:
            return 6;
        case Kvasir::IRQ::systick_irqn:
            return 15;
        }
    };

    template <Kvasir::nvic::irq_number_t irq_n>
    inline void enable()
    {
        using irq = Kvasir::nvic::irq<irq_n>;
        apply(
            write(irq::setena, true),
            write(irq::ipr, priorities(irq_n)));
    }
}