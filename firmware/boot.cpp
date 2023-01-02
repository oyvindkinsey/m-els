#include "stm32f103xb.h"

extern "C" void SystemInit() {
  RCC->CR |= RCC_CR_HSEON; // HSE clock enable
  while (!(RCC->CR & RCC_CR_HSERDY)) {} // wait for HSERDY flag to be set

  RCC->CFGR &= ~RCC_CFGR_HPRE_Msk; // set the AHB prescaler to 0, not divided
  RCC->CFGR &= ~RCC_CFGR_PPRE2_Msk; // set the APB high-speed prescaler to 0, not divided
  RCC->CFGR &= ~RCC_CFGR_PPRE1_Msk; // reset
  RCC->CFGR |= RCC_CFGR_PPRE1_DIV4; // set the  APB low-speed prescaler to 4
  RCC->CFGR |= RCC_CFGR_PLLSRC; //  select PREDIV1 as the PLL input clock
  RCC->CFGR &= ~RCC_CFGR_PLLMULL_Msk; // reset
  RCC->CFGR |= RCC_CFGR_PLLMULL9; // PLL input clock x 9

  RCC->CR |= RCC_CR_PLLON; // PLL enable
  while (!(RCC->CR & RCC_CR_PLLRDY)) {} // wait for PLLRDY flag to be set

  FLASH->ACR &= ~FLASH_ACR_LATENCY_Msk; // reset
  FLASH->ACR |= FLASH_ACR_LATENCY_1; // set LATENCY to two wait states, 48MHz < SYSCLK ≤ 72MHz

  RCC->CFGR &= ~RCC_CFGR_SW_Msk;
  RCC->CFGR |= RCC_CFGR_SW_PLL; // PLL selected as system clock
  while (!(RCC->CFGR & RCC_CFGR_SWS_PLL)) {} // wait for the system clock to switch to PLL

  // enable peripherals
  RCC->AHBENR |= RCC_AHBENR_DMA1EN;
  RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
  RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
  RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
  RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
  RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
  RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
  RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
  RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
  RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

  SysTick->LOAD = 9000 - 1; // set SysTick timer to 1ms
  SysTick->CTRL &= ~SysTick_CTRL_CLKSOURCE_Msk; // use external clock (sets div = 8)
  SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk; // enable the systick exception request
  SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk; // enable SysTick

  NVIC_SetPriority(SysTick_IRQn, 15); // set the system priority to 15
}
