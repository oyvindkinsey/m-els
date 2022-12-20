#pragma once

#include <boost/rational_minimal.hpp>
#include <Chip/STM32F103xx.hpp>
#include <chrono>
#include <Register/Register.hpp>

namespace constants
{
    using namespace std::chrono_literals;
    constexpr auto onesec_in_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(1s);

    using Rational = boost::rational<unsigned>;
    using gearing_ratio_t = std::pair<uint16_t, uint16_t>;

    constexpr uint64_t CPU_Clock_Freq_Hz = 72 * std::mega::num;
    constexpr uint16_t min_timer_capture_count = 5;

    // leadscrew
    Rational leadscrew_pitch{2, 1};

    // encoder
    constexpr uint16_t encoder_resolution{2400u};
    constexpr gearing_ratio_t encoder_gearing{1, 1};

    // stepper
    constexpr uint16_t stepper_full_steps{200u};
    constexpr uint16_t stepper_micro_steps{10};
    constexpr gearing_ratio_t stepper_gearing{1, 1};
    constexpr unsigned step_pulse_ns{2500};
    constexpr unsigned step_dir_hold_ns{5000};
    constexpr bool invert_step_pin{false};
    constexpr bool invert_dir_pin{true};

    namespace pins
    {
        using led_pin = Kvasir::gpio::Pin<Kvasir::gpio::PC, 13>;

        using step_pin = Kvasir::gpio::Pin<Kvasir::gpio::PB, 0>;
        using dir_pin = Kvasir::gpio::Pin<Kvasir::gpio::PB, 1>;

        using enc_A = Kvasir::gpio::Pin<Kvasir::gpio::PA, 8>;
        using enc_B = Kvasir::gpio::Pin<Kvasir::gpio::PA, 9>;
        using tim1_ch3 = Kvasir::gpio::Pin<Kvasir::gpio::PA, 10>;

        using tim2_ch2 = Kvasir::gpio::Pin<Kvasir::gpio::PA, 1>;

        // Serial2, for debugging
        using uart2_TX = Kvasir::gpio::Pin<Kvasir::gpio::PA, 2>;
        // using uart2_RX = Kvasir::gpio::Pin<Kvasir::gpio::PA, 3>;

        using uart1_remapped_TX = Kvasir::gpio::Pin<Kvasir::gpio::PB, 6>;
        using uart1_remapped_RX = Kvasir::gpio::Pin<Kvasir::gpio::PB, 7>;
    }

    constexpr uint16_t usart_brr_val(uint64_t usart_clock_freq, uint32_t baud_rate)
    {
        return usart_clock_freq / baud_rate;
    }
}