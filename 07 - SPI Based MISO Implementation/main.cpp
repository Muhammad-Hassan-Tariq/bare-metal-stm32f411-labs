/**
 * @file    main.cpp
 * @author  [Muhammad Hassan Tariq/ https://github.com/Muhammad-Hassan-Tariq]
 * @brief   Bare-metal implementation of SPI in MISO
 * @details Implemented SPI Based Communication with Arduino as Controller
            100Khz Mode configured on SPI1.
            // Tabular Form
            PIN B3         PCLK | Alternate Function [AF5]
            PIN B4         MISO | Alternate Function [AF5]
            PIN B5         MOSI | Alternate Function [AF5]
 * @date    2026-02-08
 * @note    Target: STM32F411CEU6 (Black Pill)
 */

#include "../platform/drivers/stm32f411xe.h"
#include <cstdint>
#define SPI_TIMEOUT_VAL 100000

/******************************************************************************
  Function: Initialize SPI
******************************************************************************/
void init_spi() {
  // Initialize Clocks
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
  RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

  // Set AF Mode, High Speed Mode, AF 5
  GPIOB->MODER &= ~(GPIO_MODER_MODE3 | GPIO_MODER_MODE4 | GPIO_MODER_MODE5);
  GPIOB->MODER |= (GPIO_MODER_MODE3_1 | GPIO_MODER_MODE4_1 | GPIO_MODER_MODE5_1);
  GPIOB->OSPEEDR |= (GPIO_OSPEEDER_OSPEEDR3_1 | GPIO_OSPEEDER_OSPEEDR4_1 | GPIO_OSPEEDER_OSPEEDR5_1);
  GPIOB->AFR[0] &= ~(GPIO_AFRL_AFSEL3_Msk | GPIO_AFRL_AFSEL4_Msk | GPIO_AFRL_AFSEL5_Msk);
  GPIOB->AFR[0] |= (0b0101 << GPIO_AFRL_AFSEL3_Pos) | (0b0101 << GPIO_AFRL_AFSEL4_Pos) | (0b0101 << GPIO_AFRL_AFSEL5_Pos);

  SPI1->CR1 &= ~SPI_CR1_SPE; // Disable SPI
  SPI1->CR1 |= SPI_CR1_MSTR; // Enable Master

  // Replace NSS pin to SSI Bit, Virtual State = 1
  SPI1->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI;

  // Baud Rate for HSI (16Mhz)
  // BR = f_pclk / x
  // BR = 16,000,000 / 2 -> 8,000,000 [8 Mbps]
  SPI1->CR1 &= ~SPI_CR1_BR;

  SPI1->CR1 |= SPI_CR1_SPE; // Enable SPI
}

/******************************************************************************
  Function: SPI Send Data [Byte]
******************************************************************************/
int spi_write_byte(uint8_t data) {
  int timeout = SPI_TIMEOUT_VAL; // Initialize Timeout

  // Wait Until SPI is busy
  while (!(SPI1->SR & SPI_SR_TXE) && --timeout);
  if (timeout == 0)
    return -1;

  SPI1->DR = data; // Write Data

  timeout = SPI_TIMEOUT_VAL; // Initialize Timeout
  // Wait for the byte to finish
  while (!(SPI1->SR & SPI_SR_RXNE) && --timeout);
  if (timeout == 0)
    return -2;

  // MUST READ DR to clear RXNE and prevent Overrun (OVR) errors
  uint8_t dummy = SPI1->DR;
  (void)dummy; // Compiler Hint

  return 1;
}

/******************************************************************************
  Function: Initialize Builtin-LED
******************************************************************************/
void init_builtin_led() {
  RCC->AHB1ENR |= (1 << 2);

  // 2. Configure PC13 as Output (MODER Register)
  // Clear mode bits for 13, then set to 01 (General purpose output)
  GPIOC->MODER &= ~(3 << (13 * 2));
  GPIOC->MODER |= (1 << (13 * 2));
}

/******************************************************************************
  Function: Main Function
******************************************************************************/
int main() {
  init_builtin_led();
  init_spi();
  char data[8] = "Hassan";
  uint8_t idx = 0;
  while (true) {
    spi_write_byte(data[idx]);
    idx = (idx == 6) ? 0 : idx + 1;

    GPIOC->ODR ^= (1 << 13);                   // Toggle Builtin-LED
    for (volatile int i = 0; i < 100000; i++); // Simple delay loop for visibility
  }
}
