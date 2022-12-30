#include "stm32f103xb.h"
#include "../constants.hpp"

#include <array>

namespace container {

  template <typename Data, unsigned Size, typename Index>
  class fixed_size_circular_buffer {
    static_assert((Size& (Size - 1)) == 0, "Need power of 2 size");

    static constexpr unsigned Mask = Size - 1;
    std::array<Data, Size> buffer{};
    volatile Index read_index = 0;
    volatile Index write_index = 0;

  public:
    void push(Data data) {
      buffer[write_index & Mask] = data;
      ++write_index;
    }

    Data pop() {
      auto result = buffer[read_index & Mask];
      ++read_index;
      return result;
    }

    bool empty() const {
      return read_index == write_index;
    }

    void skip(Index count) {
      read_index += count;
    }

  };
}

namespace devices {
  inline static volatile unsigned command_length = 0;

  template <unsigned ClkFreq = constants::CPU_Clock_Freq_Hz, unsigned BaudRate = 115200>


  struct hmi {

    enum class hmi_event: uint8_t {
      none = 0
    };

    static void init() {
      AFIO->MAPR |= AFIO_MAPR_USART1_REMAP;

      // tx
      GPIOB->CRL &= ~(GPIO_CRL_CNF6_Msk | GPIO_CRL_MODE6_Msk); // reset
      GPIOB->CRL |= GPIO_CRL_CNF6_1; // Alternate function output Push-pull
      GPIOB->CRL |= GPIO_CRL_MODE6_1; // Output mode, max speed 2MHz

      // rx
      GPIOB->CRL &= ~(GPIO_CRL_CNF7_Msk | GPIO_CRL_MODE7_Msk); // reset
      GPIOB->CRL |= GPIO_CRL_CNF7_0; // Floating input 

      // usart
      USART1->BRR = ClkFreq / BaudRate; // baud rate register
      USART1->CR1 |= USART_CR1_RXNEIE; // RXNE interrupt enable
      USART1->CR1 |= USART_CR1_RE; // Receiver enable
      USART1->CR1 |= USART_CR1_TE; // Transmitter enable
      USART1->CR1 |= USART_CR1_UE; // USART enable

      NVIC_EnableIRQ(USART1_IRQn);
    }

    static inline void process_interrupt() {
      char c = USART1->DR;
      if (c == 13) {// carriage return
        char* command = new char[command_length];
        for (auto i = 0; i < command_length; i++) {
          command[i] = read_buf.pop();
        }
        command_buf.push(command);
        command_length = 0;
      } else {
        read_buf.push(c);
        ++command_length;
      }
    }

    static inline char* process() {
      while (!command_buf.empty()) {
        return command_buf.pop();
      }
      return NULL;
    }

    static void send_rpm(const std::string_view& desc, uint16_t val, bool dir) {
      send_packet(
        std::sprintf(
          out_buf.begin(),
          "thread=%s, rpm=%u, fwd=%d\r\n",
          std::string(desc).c_str(),
          val,
          dir));
    }

  private:
    inline static std::array<char, 256> out_buf{};
    inline static container::fixed_size_circular_buffer<char, 256, uint16_t> read_buf{};
    inline static container::fixed_size_circular_buffer<char*, 8, uint16_t> command_buf{};

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