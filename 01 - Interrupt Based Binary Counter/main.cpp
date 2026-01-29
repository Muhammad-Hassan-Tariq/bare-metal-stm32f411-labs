/**
 * @file    main.cpp
 * @author  [Muhammad Hassan Tariq/ https://github.com/Muhammad-Hassan-Tariq]
 * @brief   Bare-metal implementation of EXTI0
 * @details Implemented binary count LED Pattern made of 05 LEDs/ GPIO-Output
            which increments count via input from Push Button on PIN B0 on
            falling trigger
            // Tabular Form
            PIN A0-A4      Output
            PIN B0         Input
            EXTI0          PIN B0 & Falling Trigger
 * @date    2026-01-10
 * @note    Target: STM32F411CEU6 (Black Pill)
 */

#include "../platform/drivers/stm32f411xe.h"

int count = 0; // Variable to store count for Push Button

void led_pattern() {
  for (int i = 0; i < 5; i++) {
    uint32_t targetState =
        (count >> i) & 1; // Isolate the i-th bit of count (0 or 1)
    uint32_t actualState =
        (GPIOA->IDR >> i) &
        1; // Isolate the i-th bit of the physical pin (0 or 1)

    // If they are different, toggle the output
    if (targetState ^ actualState) {
      GPIOA->ODR ^= (1U << i);
    }
  }
}

extern "C" void EXTI0_IRQHandler(void) {
  if (EXTI->PR & EXTI_PR_PR0) {
    EXTI->PR =
        EXTI_PR_PR0; // Reset Pending Register so Interrupt doesn't retrigger
    count++;         // Increment Count

    // This is for demonstration purposes. In real projects try to keep
    // Interrupt Handlers as minimal as possible Best approach is to just update
    // flags here & carry out remaining process inside main function
    led_pattern();
  }
}

inline void leds_init() {
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; // Enable PORT A Clock

  // For each LED which in our case is 5 Set it to OUTPUT Mode
  for (int i = 0; i < 5; i++) {
    GPIOA->MODER &= ~(3U << (i * 2));
    GPIOA->MODER |= (1U << (i * 2));
  }
}

int main(void) {
  leds_init();
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; // Enable PORT B clock

  GPIOB->MODER &= ~(3U << (0 * 2)); // Input mode (00)
  GPIOB->PUPDR |= (1U << (0 * 2));  // Pull-up (01)

  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;         // Enable Clock for SYSCFG
  SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0;   // Clear existing bits for Pin B0
  SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI0_PB; // Set Port B for Pin B0

  EXTI->IMR |= EXTI_IMR_MR0;   // Interrupt Mask for Line 0
  EXTI->FTSR |= EXTI_FTSR_TR0; // Falling Edge Trigger Interrupt

  NVIC_EnableIRQ(EXTI0_IRQn); // Enable NVIC Interrupt for EXTI0
  while (1) {
  }
}
