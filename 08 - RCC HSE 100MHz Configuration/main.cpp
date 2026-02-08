/**
 * @file    main.cpp
 * @author  [Muhammad Hassan Tariq/ https://github.com/Muhammad-Hassan-Tariq]
 * @brief   Bare-metal implementation of RCC HSE 100MHz Configuration
 * @details Implemented 25 Mhz HSE boost to 100 Mhz
 * @date    2026-02-08
 * @note    Target: STM32F411CEU6 (Black Pill)
 */

#include "../platform/drivers/stm32f411xe.h"

/******************************************************************************
  Function: System Clock Configuration
******************************************************************************/
void SystemClock_Config(void) {
  RCC->APB1ENR |= RCC_APB1ENR_PWREN; // Enable Power Peripheral

  // Enable HSE
  RCC->CR |= RCC_CR_HSEON;
  while (!(RCC->CR & RCC_CR_HSERDY));

  // Set VOS to 11b (Scale 1: up to 100MHz)
  PWR->CR |= (0b11 << PWR_CR_VOS_Pos);

  // Flash latency & Performance Boost [Instruction Cache, Prefetch & Data Cache]
  FLASH->ACR |= FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;
  FLASH->ACR |= FLASH_ACR_LATENCY_3WS;

  // Configure PLL
  RCC->PLLCFGR = 0;
  RCC->PLLCFGR |= (25 << RCC_PLLCFGR_PLLM_Pos);  // PLL_M = 25
  RCC->PLLCFGR |= (200 << RCC_PLLCFGR_PLLN_Pos); // PLL_N = 200
  RCC->PLLCFGR |= (0 << RCC_PLLCFGR_PLLP_Pos);   // PLL_P = 2
  RCC->PLLCFGR |= RCC_PLLCFGR_PLLSRC_HSE;        // HSE as source

  // Prescalers
  RCC->CFGR |= RCC_CFGR_HPRE_DIV1;  // AHB = 100 MHz
  RCC->CFGR |= RCC_CFGR_PPRE1_DIV2; // APB1 = 50 MHz
  RCC->CFGR |= RCC_CFGR_PPRE2_DIV1; // APB2 = 100 MHz

  // Enable PLL
  RCC->CR |= RCC_CR_PLLON;
  while (!(RCC->CR & RCC_CR_PLLRDY));

  // Switch to PLL
  RCC->CFGR &= ~RCC_CFGR_SW;
  RCC->CFGR |= RCC_CFGR_SW_PLL;

  // Wait Until Clock Switches to PLL
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}

/******************************************************************************
  Function: Initialize Builtin-LED
******************************************************************************/
void init_builtin_led() {
  RCC->AHB1ENR |= (1 << 2);

  // Configure PC13 as Output (MODER Register)
  // Clear mode bits for 13, then set to 01 (General purpose output)
  GPIOC->MODER &= ~(3 << (13 * 2));
  GPIOC->MODER |= (1 << (13 * 2));
  GPIOC->ODR |= (1 << 13);
}

/******************************************************************************
  Function: Main Function
******************************************************************************/
int main() {
  init_builtin_led();
  for (int i = 0; i < 10; i++) {
    GPIOC->ODR ^= (1 << 13);                   // Toggle Builtin-LED
    for (volatile int j = 0; j < 500000; j++); // Simple delay loop for visibility
  }

  SystemClock_Config();
  // LED should Blink Faster due to Faster Clock Speed
  while (true) {
    GPIOC->ODR ^= (1 << 13);                   // Toggle Builtin-LED
    for (volatile int i = 0; i < 500000; i++); // Simple delay loop for visibility
  }
}
