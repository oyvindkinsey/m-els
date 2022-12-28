
#include <algorithm>
#include <array>
#include <Chip/STM32F103xx.hpp>
#include <chrono>
#include <cstdio>
#include <limits>
#include <optional>
#include <Register/Register.hpp>
#include <Register/Utility.hpp>
#include <string_view>

#include "components/gear.hpp"
#include "components/timer.hpp"
#include "configuration.hpp"
#include "devices/debug.hpp"
#include "devices/encoder_pulse_duration.hpp"
#include "devices/encoder.hpp"
#include "devices/hmi.hpp"
#include "devices/rpm_counter.hpp"
#include "devices/step_gen.cpp"
#include "interrupts.hpp"
#include "thread_list.hpp"
#include "threads.hpp"

#ifdef DEBUG
#include "devices/serial.hpp"
#endif

namespace systick_state {
  volatile uint8_t rpm_sample_prescale_count = 0;
}

namespace ui {
  volatile bool rpm_update = false;
  volatile bool rpm_report = false;
}

namespace control {

  enum class State: uint8_t {
    stopped,
    in_sync,
    ramping,
  };

  volatile State state = State::in_sync; // TODO: default should be OFF
}

extern "C"
{ // interrupt handlers
  void SysTick_Handler() { // Called every 1 ms
    timer::process_interrupt();

    using rpm_sampler = devices::rpm_counter<>;
    using namespace systick_state;
    if (ui::rpm_report) {
      auto psc = rpm_sample_prescale_count;
      if (++psc == rpm_sampler::Sampling_period) {
        if (rpm_sampler::process_sample(devices::encoder::get_count())) {
          ui::rpm_update = true;
        }
        rpm_sample_prescale_count = 0;
      } else {
        rpm_sample_prescale_count = psc;
      }
    }
    if ((timer::milliseconds & 1023) == 0) {
      devices::debug::toggle_led();
    }
  }

  void TIM1_CC_IRQHandler() {
    using namespace devices;
    auto enc = encoder::get_count();
    bool dir = step_gen::get_direction();
    const bool fwd = encoder::is_cc_fwd_interrupt();
    encoder::clear_cc_interrupt();
    using namespace gear;
    if (fwd) {
      encoder::trigger_clear();
      state.err = range.next.error;
      range.next_jump(dir, enc);
      step_gen::set_delay(phase_delay(encoder_pulse_duration::last_duration(), range.next.error));
      encoder::trigger_restore();
    } else { // Change direction, setup delayed pulse and do manual trigger
      dir = !dir;
      step_gen::change_direction(dir);
      encoder::trigger_manual_pulse();
      state.err = range.prev.error;
      range.next_jump(dir, enc);
    }
    encoder::update_channels(range.next.count, range.prev.count);
  }

  void TIM2_IRQHandler() {
    devices::encoder_pulse_duration::process_interrupt();
  }

  void TIM3_IRQHandler() {
    using devices::step_gen;
    step_gen::process_interrupt();
    gear::state.output_position += step_gen::get_direction() ? 1 : -1;
  }

  void USART1_IRQHandler() {
    devices::hmi<>::process_interrupt();
  }
} // extern "C"

Configuration config{};

int main() {
  using namespace devices;

#ifdef DEBUG
  Serial2<>::init(); // Used as console
#endif

  devices::debug::init();

  step_gen::init();
  step_gen::configure(constants::step_dir_hold_ns, constants::step_pulse_ns,
    constants::invert_step_pin, constants::invert_dir_pin);

  gear::configure(config.calculate_ratio(), 0);

  encoder::init();
  encoder::update_channels(gear::range.next.count, gear::range.prev.count);

  encoder_pulse_duration::init();

  using display = hmi<>;
  display::init();

  ui::rpm_report = true;

  auto f_check_thread = [&](int16_t index) -> uint8_t {
    return config.verify_thread<encoder::CounterValue>(index);
  };

  while (true) {
    if (ui::rpm_update && ui::rpm_report) {
      ui::rpm_update = false;
      // todo send rpm
      display::send_rpm(
        rpm_counter<>::get_rpm(constants::encoder_resolution),
        step_gen::get_direction());
    }
  }

  return 0;
}
