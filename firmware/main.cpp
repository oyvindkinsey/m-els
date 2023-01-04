
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <limits>
#include <optional>
#include <string_view>

#include "components/gear.hpp"
#include "devices/encoder.hpp"
#include "devices/hmi.hpp"
#include "devices/i2c.hpp"
#include "devices/step_gen.hpp"
#include "threads.hpp"

namespace systick_state {
  volatile uint8_t rpm_sample_prescale_count = 0;
  inline volatile unsigned int milliseconds = 0;
}

namespace devices {
  struct debug {
    static void init() {
      GPIOC->CRH &= ~GPIO_CRH_CNF13_Msk | GPIO_CRH_MODE13_Msk; // general purpose output push-pull
      GPIOC->CRH |= GPIO_CRH_MODE13_1; // Output mode, max speed 2MHz
    }

    static void toggle_led() {
      GPIOC->ODR ^= GPIO_ODR_ODR13_Msk;
    }
  };

  template <uint8_t Period_ms = 10, uint8_t Samples = 16>

  struct rpm_counter {
    static constexpr uint16_t periods_per_min = 60000 / Period_ms;
    static constexpr uint8_t Sampling_period = Period_ms;

    volatile inline static uint16_t last_reading = 0;
    volatile inline static uint8_t sample_index = 0;
    volatile inline static unsigned running_sum = 0;
    volatile inline static uint16_t sum = 0;

    static inline uint16_t convert_sample(uint16_t c) {
      auto p = last_reading;
      uint16_t d = (c > p) ? (c - p) : (p - c);
      if (d > std::numeric_limits<uint16_t>::max() / 2) {
        d = std::numeric_limits<uint16_t>::max() - d;
      }
      last_reading = c;
      return d;
    }

    static bool process_sample(uint16_t current_reading) {
      running_sum += convert_sample(current_reading);
      auto i = sample_index;
      if ((++i) == Samples) {
        sum = running_sum;
        sample_index = 0;
        running_sum = 0;
        return true;
      } else {
        sample_index = i;
        return false;
      }
    }

    static uint16_t get_rpm(uint16_t encoder_resolution) {
      auto rpm = (sum * periods_per_min) / (encoder_resolution * Samples);
      return static_cast<uint16_t>(rpm);
    }
  };
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
    using namespace systick_state;
    ++milliseconds;

    using rpm_sampler = devices::rpm_counter<>;
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

  void USART1_IRQHandler() {
    if (USART1->SR & USART_SR_IDLE) {
      (void)USART1->DR; // clear idle flag
      devices::hmi::process_idle_interrupt();
    }
  }

  void DMA1_Channel5_IRQHandler() {
    if (DMA1->ISR & DMA_ISR_TCIF5) {
      DMA1->IFCR = DMA_IFCR_CTCIF5;
      devices::hmi::process_tc_interrupt();
    }
  }

  void I2C1_EV_IRQHandler() {
    devices::i2c::I2C1_EV_IRQHandler();

  }

  void DMA1_Channel7_IRQHandler() {
    devices::i2c::DMA1_Channel7_IRQHandler();
  }
} // extern "C"

int main() {
  using namespace devices;
  using namespace threads::literals;

  devices::debug::init();

  step_gen::init();
  step_gen::configure(
    constants::step_dir_hold_ns,
    constants::step_pulse_ns,
    constants::invert_step_pin,
    constants::invert_dir_pin);

  gear::configure(2.00_mm, 0);

  encoder::init();
  encoder::update_channels(
    gear::range.next.count,
    gear::range.prev.count);

  hmi::init();
  i2c::init();

  ui::rpm_report = true;

  while (true) {
    if (ui::rpm_update && ui::rpm_report) {
      ui::rpm_update = false;
      hmi::send_info(
        gear::get_pitch_info().pitch_str,
        rpm_counter<>::get_rpm(constants::encoder_resolution),
        step_gen::get_direction());
    }

    auto command = hmi::process();
    if (command != NULL) {
      switch (command[0]) {
      case 0x31:
        gear::configure(1.00_mm, encoder::get_count());
        break;
      case 0x32:
        gear::configure(1.50_mm, encoder::get_count());
        break;
      case 0x33:
        gear::configure(2.00_mm, encoder::get_count());
        break;
      default:
        break;
      }
    }
  }

  return 0;
}
