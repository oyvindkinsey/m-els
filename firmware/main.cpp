
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <limits>
#include <optional>
#include <string_view>

#include "components/gear.hpp"
#include "devices/encoder.hpp"
#include "devices/i2c.hpp"
#include "devices/uart.hpp"
#include "devices/step_gen.hpp"
#include "devices/rpm.hpp"

#define FLAGS_STEPPER_EN 0x80
#define FLAGS_RPM_EN 0x40

namespace systick_state {
  volatile uint8_t rpm_sample_prescale_count = 0;
  inline volatile unsigned int milliseconds = 0;
}

namespace devices {
  struct debug {
    static void init() {
      GPIOC->CRH &= ~(GPIO_CRH_CNF13_Msk | GPIO_CRH_MODE13_Msk); // general purpose output push-pull
      GPIOC->CRH |= GPIO_CRH_MODE13_1; // Output mode, max speed 2MHz
    }

    static void toggle_led() {
      GPIOC->ODR ^= GPIO_ODR_ODR13_Msk;
    }
  };
}

namespace ui {
  volatile bool rpm_update = false;
}

namespace control {

  enum class State : uint8_t {
    stopped,
    in_sync,
    ramping,
  };

  volatile State state = State::in_sync; // TODO: default should be OFF
}

extern "C"
{ // interrupt handlers
  void SysTick_Handler() { // Called every 1 ms
    using namespace systick_state;
    ++milliseconds;

    using rpm_sampler = devices::rpm_counter<>;
    auto psc = rpm_sample_prescale_count;
    if (++psc == rpm_sampler::Sampling_period) {
      rpm_sampler::process_sample(devices::encoder::get_count());
      rpm_sample_prescale_count = 0;
    } else {
      rpm_sample_prescale_count = psc;
    }
    if ((milliseconds & 1023) == 0) {
      devices::debug::toggle_led();
    }
  }

  void TIM1_CC_IRQHandler() {
    using namespace devices;
    auto enc = encoder::get_count();
    bool dir = step_gen::get_direction();
    const bool fwd = encoder::is_cc_fwd_interrupt();
    encoder::clear_cc_interrupt();
    if (i2c::reg_settings.mode != 0x1) {
      // todo: we need to re-initialize when re-enabling
      return;
    }
    using namespace gear;
    if (fwd) {
      encoder::trigger_clear();
      state.err = range.next.error;
      range.next_jump(dir, enc);
      step_gen::set_delay(phase_delay(encoder::last_duration(), range.next.error));
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
    devices::encoder::process_interrupt();
  }

  void TIM3_IRQHandler() {
    using devices::step_gen;
    step_gen::process_interrupt();
    gear::state.output_position += step_gen::get_direction() ? 1 : -1;
  }

  void DMA1_Channel5_IRQHandler() {
    devices::i2c::DMA1_Channel5_IRQHandler();
  }

  void I2C2_EV_IRQHandler() {
    devices::i2c::I2C2_EV_IRQHandler();

  }

} // extern "C"

int main() {
  using namespace devices;

  devices::debug::init();
  step_gen::init();
  encoder::init();
  encoder::update_channels(
    gear::range.next.count,
    gear::range.prev.count);

  uart::init();
  i2c::init();

  uint8_t num = 0;
  uint8_t denom = 1;
  char mode = 0;

  while (true) {
    if (i2c::reg_settings.gear_num != num || i2c::reg_settings.gear_denom != denom) {
      num = i2c::reg_settings.gear_num;
      denom = i2c::reg_settings.gear_denom;
      gear::configure({ num, denom }, encoder::get_count());

      char buffer[16];
      uart::write(buffer, sprintf(buffer, "gears changed to %d/%d\n", num, denom));
    }

    if (i2c::reg_settings.mode != mode) {
      mode = i2c::reg_settings.mode;
      if (mode == 0x1) {
        step_gen::configure(
          i2c::reg_configuration.stepper_change_dwell_ns,
          i2c::reg_configuration.stepper_pulse_length_ns,
          i2c::reg_configuration.stepper_flags & 0x1,
          i2c::reg_configuration.stepper_flags & 0x2);
        char buffer[16];
        uart::write(buffer, sprintf(buffer, "set mode to %d\n", mode));
      }
    }
  }

  return 0;
}
