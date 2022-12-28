#include "stm32f103xb.h"
#include "../constants.hpp"

namespace devices {
  template <unsigned ClkFreq = constants::CPU_Clock_Freq_Hz, unsigned BaudRate = 115200>

  struct hmi {

    static void init() {
      AFIO->MAPR |= AFIO_MAPR_USART1_REMAP;

      // tx
      GPIOB->CRL &= ~(GPIO_CRL_CNF6_Msk | GPIO_CRL_MODE6_Msk); // reset
      GPIOB->CRL |= GPIO_CRL_CNF6_1; // Alternate function output Push-pull
      GPIOB->CRL |= GPIO_CRL_MODE6_1; // Output mode, max speed 2MHz

      // rx
      GPIOB->CRL &= ~(GPIO_CRL_CNF7_Msk | GPIO_CRL_MODE7_Msk); // reset
      GPIOB->CRL |= GPIO_CRL_CNF6_0; // Floating input 

      // usart
      USART1->BRR = ClkFreq / BaudRate; // baud rate register
      USART1->CR1 |= USART_CR1_RXNEIE; // RXNE interrupt enable
      USART1->CR1 |= USART_CR1_RE; // Receiver enable
      USART1->CR1 |= USART_CR1_TE; // Transmitter enable
      USART1->CR1 |= USART_CR1_UE; // USART enable

    }

    static inline void process_interrupt() {
      auto c = USART1->DR;
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
      while (!(USART1->SR & USART_SR_TXE)) {}
      USART1->DR = c;
    }

    static void send_string_packet(const std::string_view& s) {
      for (char c : s) {
        send_char(c);
      }
    }
  };
}