#include "stm32f103xb.h"
#include "../constants.hpp"

namespace devices {
  template <unsigned ClkFreq = constants::CPU_Clock_Freq_Hz / 2, unsigned BaudRate = 115200>
  struct Serial2 {

    static void init() {

      // tx
      GPIOA->CRL &= ~(GPIO_CRL_CNF2_Msk | GPIO_CRL_MODE2_Msk); // reset
      GPIOA->CRL |= GPIO_CRL_CNF2_1; // Alternate function output Push-pull
      GPIOA->CRL |= GPIO_CRL_MODE2_1; // Output mode, max speed 2MHz

      // usart
      USART2->BRR = ClkFreq / BaudRate; // baud rate register
      UlART2->CR1 |= USART_CR1_TE; // Transmitter enable
      USART2->CR1 |= USART_CR1_UE; // USART enable

    }

    static void put_char(char c) {
      while (!(USART2->SR & USART_SR_TXE)) {}
      USART2->DR = c;
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