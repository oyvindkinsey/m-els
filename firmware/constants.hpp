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
}