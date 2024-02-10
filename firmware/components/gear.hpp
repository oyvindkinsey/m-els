#pragma once

#include <stdint.h>
#include "../constants.hpp"
#include "../devices/i2c.hpp"

namespace gear {
  using Rational = boost::rational<unsigned>;

  struct State {
    int D, N; // pulse ratio : N/D
    int err = 0;
    int output_position = 0;
  };

  volatile State state = { 4, 1 };

  struct Jump {
    uint16_t count;
    uint16_t delta;
    int error;
  };

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnarrowing"
  // Narrowing comes due to integer promotion in arithmetic operations
  // k, the encoder count delta for the next step pulse, should fit within a short integer

  inline Jump next_jump_forward(int d, int n, int e, uint16_t count) {
    uint16_t k = (d - 2 * e + 2 * n - 1) / (2 * n);
    return { count + k, k, e + k * n - d };
  }

  inline Jump next_jump_reverse(int d, int n, int e, uint16_t count) {
    uint16_t k = 1 + ((d + 2 * e) / (2 * n));
    return { count - k, k, e - k * n + d };
  }

  struct Range {
    Jump next{}, prev{};

    void next_jump(bool dir, uint16_t count) {
      int d = state.D, n = state.N, e = state.err;
      if (!dir) {
        next = next_jump_forward(d, n, e, count);
        prev = { count - 1, 1u, next.error + d - n };
      } else {
        next = next_jump_reverse(d, n, e, count);
        prev = { count + 1, 1u, next.error - d + n };
      }
    }
  };
#pragma GCC diagnostic pop

  Range range;

  Rational calculate_ratio_for_pitch(const uint8_t& num, const uint8_t& denom) {
    using namespace devices;
    Rational encoder = {
      i2c::reg_configuration.encoder_resolution,
      1
    };
    Rational steps_per_rev = {
      i2c::reg_configuration.stepper_resolution,
      1
    };

    Rational pitch{ num, denom };

    return (pitch / constants::leadscrew_pitch) * steps_per_rev / encoder;
  }
  void configure(const uint8_t& num, const uint8_t& denom, uint16_t start_position) {
    auto ratio = calculate_ratio_for_pitch(num, denom);

    state.D = ratio.denominator();
    state.N = ratio.numerator();
    state.err = 0;
    range.next = next_jump_forward(ratio.denominator(), ratio.numerator(), 0, start_position);
    range.prev = next_jump_reverse(ratio.denominator(), ratio.numerator(), 0, start_position);
  }

  inline unsigned phase_delay(uint16_t input_period, int e) {
    if (e < 0)
      e = -e;
    return (input_period * e) / (state.N);
  }

}