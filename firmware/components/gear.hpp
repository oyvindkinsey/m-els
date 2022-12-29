#pragma once

#include <stdint.h>
#include "../threads.hpp"
#include "../constants.hpp"

namespace gear {
  using Rational = threads::Rational;

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
  threads::pitch_info pitchInfo;


  Rational calculate_ratio_for_pitch(const Rational& pitch) {
    Rational encoder = {
        constants::encoder_resolution * constants::encoder_gearing.first,
        constants::encoder_gearing.second };
    Rational steps_per_rev = {
        constants::stepper_full_steps * constants::stepper_micro_steps *
            constants::stepper_gearing.first,
        constants::stepper_gearing.second };

    return (pitch / constants::leadscrew_pitch) * steps_per_rev / encoder;
  }
  void configure(const threads::pitch_info& pitch, uint16_t start_position) {
    pitchInfo = pitch;
    auto ratio = calculate_ratio_for_pitch(pitch.value);

    state.D = ratio.denominator();
    state.N = ratio.numerator();
    state.err = 0;
    range.next = next_jump_forward(ratio.denominator(), ratio.numerator(), 0, start_position);
    range.prev = next_jump_reverse(ratio.denominator(), ratio.numerator(), 0, start_position);
  }

  threads::pitch_info get_pitch_info() {
    return pitchInfo;
  }

  inline unsigned phase_delay(uint16_t input_period, int e) {
    if (e < 0)
      e = -e;
    return (input_period * e) / (state.N);
  }

}