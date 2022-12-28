
#include <Chip/STM32F103xx.hpp>
#include <Register/Register.hpp>

#include "../constants.hpp"

namespace devices {
  template <unsigned ClkFreq = constants::CPU_Clock_Freq_Hz / 2, unsigned BaudRate = 115200>
  struct Serial2 {
    using pin_TX = constants::pins::uart2_TX;
    // using pin_RX = constants::pins::uart2_RX;

    static void init() {
      // Pins
      using namespace Kvasir;
      apply(write(pin_TX::cr::mode, gpio::PinMode::Output_2Mhz),
        write(pin_TX::cr::cnf, gpio::PinConfig::Output_alternate_push_pull));
      apply(write(Usart2Brr::brr_12_4, mcu::usart_brr_val(ClkFreq, BaudRate)),
        set(Usart2Cr1::te));
      apply(set(Usart2Cr1::ue));
    }

    static void put_char(char c) {
      using namespace Kvasir;
      while (!apply(read(Usart2Sr::txe)))
        ;
      apply(write(Usart2Dr::dr, c));
    }
  };
}

extern "C" {

  int _write(int fd, char* ptr, int len) {
    /* Write "len" of char from "ptr" to file id "fd"
     * Return number of char written.
     * Need implementing with UART here. */
    for (; len > 0; --len, ++ptr) {
      devices::Serial2<>::put_char(*ptr);
    }
    return len;
  }

  int _read(int fd, char* ptr, int len) {
    /* Read "len" of char to "ptr" from file id "fd"
     * Return number of char read.
     * Need implementing with UART here. */
    return len;
  }

  void _ttywrch(int ch) {
    devices::Serial2<>::put_char(ch);
  }

}