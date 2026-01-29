/**
 * @file    main.cpp
 * @author  [Muhammad Hassan Tariq/ https://github.com/Muhammad-Hassan-Tariq]
 * @brief   Bare-metal implementation of USART TX
 * @details Implemented USART based Array Transmission Mechanisim. It makes use of
            USART2 on MCU to send string "Hassan" by using senData function
            continously. SysTick timer is used to delay the Transmission which prevents
            overflow.
            // Tabular Form
            PIN A2         Output | Alternate Function [AF7]
            USART2         AF7 & TX
 * @date    2026-01-28
 * @note    Target: STM32F411CEU6 (Black Pill)
 */

#include "../platform/drivers/stm32f411xe.h"
#include <array>

volatile bool send_ready = false; // Flag for SysTick Delay

/******************************************************************************
  Function: Initialize USART2
******************************************************************************/
void init_uart() {
  // Enable Clocks USART2 & GPIOA
  RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

  // Clear & Set Mode to Alternate Function
  GPIOA->MODER &= ~GPIO_MODER_MODER2_Msk;
  GPIOA->MODER |= (0b10 << GPIO_MODER_MODER2_Pos);

  // Clear & Set AF7 to PA2
  GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL2_Msk;
  GPIOA->AFR[0] |= (7U << GPIO_AFRL_AFSEL2_Pos);

  // Set Baud Rate for HSI 16 Mhz 115200 Baud Rate
  // BRR = 16,000,000 / (16 * 115200) -> 8.680
  // Mantissa = 8  |  Fraction = 0.680 * 16 -> 11
  USART2->BRR = (8U << USART_BRR_DIV_Mantissa_Pos) | (11U << USART_BRR_DIV_Fraction_Pos);
  USART2->CR1 = USART_CR1_TE | USART_CR1_UE; // Enable Transmission & UART
}

/******************************************************************************
  Function: Send 1 Byte
******************************************************************************/
void sendByte(char byte) {
  while (!(USART2->SR & USART_SR_TXE)); // Wait till TXE
  USART2->DR = byte;                    // Send Byte
}

/******************************************************************************
  Function: Send 10 Bytes Array
******************************************************************************/
void sendData(const std::array<char, 10> &data) {
  for (char c : data) {
    if (c == '\0')
      break; // Don't send null terminator
    sendByte(c);
  }
}

/******************************************************************************
  Function: Initialize SysTick Timer
******************************************************************************/
inline void init_systick() {
  SysTick->LOAD = 16000000 - 1; // 1 second delay @ 16MHz
  SysTick->VAL = 0;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                  SysTick_CTRL_TICKINT_Msk |
                  SysTick_CTRL_ENABLE_Msk;
}

/******************************************************************************
  Function: Systick Interrupt Handler
******************************************************************************/
extern "C" void SysTick_Handler() {
  send_ready = true;
}

/******************************************************************************
  Function: Main Function
******************************************************************************/
int main() {
  init_uart();    // Init UART
  init_systick(); // Init SysTick
  std::array<char, 10> data = {"Hassan \r\n"};

  while (1) {
    if (send_ready) {
      sendData(data);
      send_ready = false; // Reset the flag so we only send ONCE per interrupt
    }
  }
}
