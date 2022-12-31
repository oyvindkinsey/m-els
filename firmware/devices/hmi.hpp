#include "stm32f103xb.h"
#include "../constants.hpp"
#include "../circular_buffer.hpp"

namespace devices {

  constexpr uint64_t BaudRate = 115200;

  struct hmi {

  private:

    volatile static inline char dma_buff_in[4];
    inline static char dma_buff_out[256];
    inline static container::fixed_size_circular_buffer<char*, 8, uint16_t> command_buf{};

    static inline void uart_dma_transmit(
      uint16_t length) {
      while (!(USART1->SR & USART_SR_TXE)) {}

      DMA1_Channel4->CCR &= ~DMA_CCR_EN;
      DMA1->IFCR |= DMA_IFCR_CTCIF4;
      DMA1_Channel4->CMAR = (uint32_t)dma_buff_out;
      DMA1_Channel4->CNDTR = length;
      DMA1_Channel4->CCR |= DMA_CCR_EN;
    }

    static inline void process_rx() {
      char* command = new char[4];
      for (auto i = 0; i < 4; i++) {
        command[i] = dma_buff_in[i];
      }
      command_buf.push(command);
    }

  public:

    static void init() {
      // Connect USART1 to alternate pins 6 and 7
      AFIO->MAPR |= AFIO_MAPR_USART1_REMAP;

      // Set up tx pin
      GPIOB->CRL &= ~(GPIO_CRL_CNF6_Msk | GPIO_CRL_MODE6_Msk); // reset
      GPIOB->CRL |= GPIO_CRL_CNF6_1; // Alternate function output Push-pull
      GPIOB->CRL |= GPIO_CRL_MODE6_1; // Output mode, max speed 2MHz

      // Set up rx pin
      GPIOB->CRL &= ~(GPIO_CRL_CNF7_Msk | GPIO_CRL_MODE7_Msk); // reset
      GPIOB->CRL |= GPIO_CRL_CNF7_0; // Floating input

      // Configure USART1
      USART1->BRR = constants::CPU_Clock_Freq_Hz / BaudRate; // baud rate register
      USART1->CR1 |= USART_CR1_RE; // Receiver enable
      USART1->CR1 |= USART_CR1_TE; // Transmitter enable
      USART1->CR1 |= USART_CR1_IDLEIE; // idle interrupt enabled
      USART1->CR3 = USART_CR3_DMAR; // DMA enable receiver
      USART1->CR3 |= USART_CR3_DMAT; // DMA enable transmitter

      // Set up DMA for RX
      DMA1_Channel5->CCR &= ~(DMA_CCR_EN | DMA_CCR_PINC_Msk | DMA_CCR_MSIZE_Msk | DMA_CCR_PSIZE_Msk | DMA_CCR_DIR_Msk);
      DMA1_Channel5->CPAR = (uint32_t)&USART1->DR; // source
      DMA1_Channel5->CMAR = (uint32_t)&dma_buff_in; // dest
      DMA1_Channel5->CCR |= DMA_CCR_MINC; // memory increment mode
      DMA1_Channel5->CCR |= DMA_CCR_CIRC; // circular mode
      DMA1_Channel5->CCR |= DMA_CCR_TCIE; // transfer complete interrupt enable
      DMA1_Channel5->CNDTR = 4; // length of data to expect
      DMA1_Channel5->CCR |= DMA_CCR_EN;

      // Set up DMA for TX - CMAR and CNDTR are set in uart_dma_transmit
      DMA1_Channel4->CCR &= ~(DMA_CCR_EN | DMA_CCR_PINC_Msk | DMA_CCR_MSIZE_Msk | DMA_CCR_PSIZE_Msk | DMA_CCR_CIRC_Msk);
      DMA1_Channel4->CPAR = reinterpret_cast<unsigned>(std::addressof(USART1->DR)); // dest
      DMA1_Channel4->CCR |= DMA_CCR_MINC; // memory increment mode
      DMA1_Channel4->CCR |= DMA_CCR_DIR; // from memory to peripheral

      // Enable interrupts
      NVIC_SetPriority(DMA1_Channel5_IRQn, 0);
      NVIC_SetPriority(DMA1_Channel4_IRQn, 0);
      NVIC_EnableIRQ(DMA1_Channel5_IRQn);
      NVIC_EnableIRQ(DMA1_Channel4_IRQn);
      NVIC_EnableIRQ(USART1_IRQn);

      // Enable USART1
      USART1->CR1 |= USART_CR1_UE; // USART enable
    }

    static inline void process_idle_interrupt() {
      // reset DMA
      DMA1_Channel5->CCR &= ~DMA_CCR_EN;
      DMA1_Channel5->CMAR = reinterpret_cast<unsigned>(std::addressof(dma_buff_in)); // dest
      DMA1_Channel5->CNDTR = 4; // length of data to expect
      DMA1_Channel5->CCR |= DMA_CCR_EN;

      process_rx();
    }

    static inline void process_tc_interrupt() {
      process_rx();
    }

    static inline char* process() {
      while (!command_buf.empty()) {
        return command_buf.pop();
      }
      return NULL;
    }

    static void send_info(const std::string_view& desc, uint16_t val, bool dir) {
      uart_dma_transmit(std::sprintf(
        dma_buff_out,
        "thread=%s, rpm=%u, fwd=%d\r\n",
        std::string(desc).c_str(),
        val,
        dir));
    }

  };
}