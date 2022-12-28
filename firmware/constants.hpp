#pragma once

#include <boost/rational_minimal.hpp>
#include <chrono>

namespace constants {
    using namespace std::chrono_literals;
    constexpr auto onesec_in_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(1s);

    using Rational = boost::rational<unsigned>;
    using gearing_ratio_t = std::pair<uint16_t, uint16_t>;

    constexpr uint64_t CPU_Clock_Freq_Hz = 72 * std::mega::num;
    constexpr uint16_t min_timer_capture_count = 5;

    // leadscrew
    Rational leadscrew_pitch{ 2, 1 };

    // encoder
    constexpr uint16_t encoder_resolution{ 2400u };
    constexpr gearing_ratio_t encoder_gearing{ 1, 1 };

    // stepper
    constexpr uint16_t stepper_full_steps{ 200u };
    constexpr uint16_t stepper_micro_steps{ 10 };
    constexpr gearing_ratio_t stepper_gearing{ 1, 1 };
    constexpr unsigned step_pulse_ns{ 2500 };
    constexpr unsigned step_dir_hold_ns{ 5000 };
    constexpr bool invert_step_pin{ false };
    constexpr bool invert_dir_pin{ true };
}