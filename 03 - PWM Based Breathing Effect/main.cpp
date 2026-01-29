/**
 * @file    main.cpp
 * @author  [Muhammad Hassan Tariq/ https://github.com/Muhammad-Hassan-Tariq]
 * @brief   Bare-metal implementation of PWM
 * @details Implemented PWM based LED breathing effect. The duty cycle goes from
            0 -> 100 and from 100 -> 0. I have used SysTick timer to control duty
            cycle.
            // Tabular Form
            PIN A0         Output | Alternate Function
            TIM2 CH1       Timer on Pin A0
 * @date    2026-01-16
 * @note    Target: STM32F411CEU6 (Black Pill)
 */

#include "../platform/drivers/stm32f411xe.h"

/******************************************************************************
  Function: Initialize TIM2 Channel 1 for PIN A0
******************************************************************************/
inline void init_tim2() {
  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; // Enable Clock

  TIM2->CR1 |= TIM_CR1_ARPE | TIM_CR1_DIR | TIM_CR1_URS;          // Buffered Auto Reload Preload | Downcounter | Overflow/ Underflow INT
  TIM2->CCMR1 |= (0b110 << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE; // PWM Mode 1 | Output Compare 1 Preload

  TIM2->CNT = 0;    // Clear Count
  TIM2->PSC = 15;   // Prescaler | Overall Config = 16 MHz / (15 + 1) = 1 MHz
  TIM2->ARR = 1000; // Auto Reload Register | Overall Config= 1 MHz / 1000 = 1 KHz
  TIM2->CCR1 = 500; // Duty Cycle 50% = 500

  TIM2->CCER |= TIM_CCER_CC1E; // Enable Channel 1 Output
  TIM2->CR1 |= TIM_CR1_CEN;    // Enable Timer
}

/******************************************************************************
  Function: Initialize SysTick Timer
******************************************************************************/
inline void init_systick() {
  SysTick->LOAD = 16000 - 1; // Reload = 16000 for 1ms @ 16 MHz
  SysTick->VAL = 0;          // Clear VAL

  // HSI Clock | Enable INT | Enable Clock
  SysTick->CTRL |= (0b1 << SysTick_CTRL_CLKSOURCE_Pos) | (0b1 << SysTick_CTRL_TICKINT_Pos) | (0b1 << SysTick_CTRL_ENABLE_Pos);

  NVIC_EnableIRQ(SysTick_IRQn);      // Enable IRQ
  NVIC_SetPriority(SysTick_IRQn, 0); // Set Highest Priotity for access to modifying CCR1 Register for Duty Cycle
}

/******************************************************************************
  Function: Initialize PIN A0 as Alternate Function
******************************************************************************/
inline void init_pa0() {
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; // Enable Clock

  GPIOA->MODER |= (0b10 << GPIO_MODER_MODER0_Pos);      // Set Alternate Function Mode
  GPIOA->OSPEEDR |= (0b11 << GPIO_OSPEEDR_OSPEED0_Pos); // Set Fastest Speed
  GPIOA->OTYPER &= ~(1 << 0);                           // Output Type Push-Pull

  GPIOA->AFR[0] &= ~(0xF << (0 * 4)); // Clear AFR
  GPIOA->AFR[0] |= (0x1 << (0 * 4));  // Set AFR to Channel 1
}

/******************************************************************************
  Function: Systick Interrupt Handler
******************************************************************************/
extern "C" void SysTick_Handler() {
  static int8_t step = 5;   // Duty Cycle Step / Leap
  static uint32_t duty = 0; // Duty Cycle to SET

  duty += step; // Increment Duty Cycle

  // If Duty Cycle exceeds 1000 flip the step to Negative & Vice Versa for 0
  if (duty >= 1000 || duty <= 0)
    step = -step;

  TIM2->CCR1 = duty; // Set Duty Cycle
}

/******************************************************************************
  Function: Main
******************************************************************************/
int main() {
  init_pa0();     // Initialize PA0
  init_tim2();    // Initialize TIM2
  init_systick(); // Initialize SysTick

  while (1) {
    __WFI(); // Wait for Interrupt
  }
}
