
#include <Chip/STM32F103xx.hpp>
#include <Register/Register.hpp>
#include <string_view>

#include "../constants.hpp"

namespace devices {
  template <unsigned ClkFreq = constants::CPU_Clock_Freq_Hz, unsigned BaudRate = 115200>

  struct hmi {

    // TODO: usarts need a generic interface
    using pin_TX = constants::pins::uart1_remapped_TX;
    using pin_RX = constants::pins::uart1_remapped_RX;

    static void init() {
      using namespace Kvasir;
      apply(set(Kvasir::AfioMapr::usart1Remap));
      apply(write(pin_TX::cr::mode, gpio::PinMode::Output_2Mhz),
        write(pin_TX::cr::cnf, gpio::PinConfig::Output_alternate_push_pull),
        write(pin_RX::cr::mode, gpio::PinMode::Input),
        write(pin_RX::cr::cnf, gpio::PinConfig::Input_floating));
      apply(write(Usart1Brr::brr_12_4, constants::usart_brr_val(ClkFreq, BaudRate)),
        set(Usart1Cr1::rxneie),
        set(Usart1Cr1::re),
        set(Usart1Cr1::te));
      apply(set(Usart1Cr1::ue));
    }

    static inline void process_interrupt() {
      using namespace Kvasir;
      auto c = apply(read(Usart1Dr::dr));
      // todo: handle c
    }

    static void send_rpm(uint16_t val, bool dir) {
      send_packet(
        std::sprintf(
          out_buf.begin(),
          "rpm=%u, fwd=%d\r\n",
          val,
          dir));
    }

  private:
    inline static std::array<char, 256> out_buf{};

    static void send_packet(int num_chars) {
      for (auto i = out_buf.begin(); num_chars > 0; --num_chars, ++i) {
        send_char(*i);
      }
    }

    static inline void send_char(char c) {
      using namespace Kvasir;
      while (!apply(read(Usart1Sr::txe)))
        ;
      apply(write(Usart1Dr::dr, c));
    }

    static void send_string_packet(const std::string_view& s) {
      for (char c : s) {
        send_char(c);
      }
    }
  };
}