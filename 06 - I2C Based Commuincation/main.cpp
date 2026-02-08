/**
 * @file    main.cpp
 * @author  [Muhammad Hassan Tariq/ https://github.com/Muhammad-Hassan-Tariq]
 * @brief   Bare-metal implementation of I2C
 * @details Implemented I2C Based Communication with Arduino as Controller
            100Khz Mode configured on I2C1.
            // Tabular Form
            PIN B6         SCL | Alternate Function [AF4]
            PIN B7         SDA | Alternate Function [AF4]
 * @date    2026-02-05
 * @note    Target: STM32F411CEU6 (Black Pill)
 */

#include "../platform/drivers/stm32f411xe.h"
#define I2C_TIMEOUT_VAL 100000

/******************************************************************************
  Function: Initialize I2C
******************************************************************************/
void init_i2c() {
  RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

  // Alternate Function Mode PB6 & PB7
  GPIOB->MODER &= ~(GPIO_MODER_MODER6_Msk | GPIO_MODER_MODER7_Msk);
  GPIOB->MODER |= (0b10 << GPIO_MODER_MODER6_Pos) | (0b10 << GPIO_MODER_MODER7_Pos);

  // Open Drain Mode
  GPIOB->OTYPER |= (1U << GPIO_OTYPER_OT6_Pos) | (1U << GPIO_OTYPER_OT7_Pos);

  // Set PB6 and PB7 to High Speed (0b11)
  GPIOB->OSPEEDR |= (0b11 << GPIO_OSPEEDR_OSPEED6_Pos) | (0b11 << GPIO_OSPEEDR_OSPEED7_Pos);

  // Alternate Function 4
  GPIOB->AFR[0] &= ~(GPIO_AFRL_AFSEL6_Msk | GPIO_AFRL_AFSEL7_Msk);
  GPIOB->AFR[0] |= (4U << GPIO_AFRL_AFSEL6_Pos) | (4U << GPIO_AFRL_AFSEL7_Pos);

  I2C1->CR1 &= ~I2C_CR1_PE;    // Disable peripheral
  I2C1->CR1 |= I2C_CR1_SWRST;  // Reset
  I2C1->CR1 &= ~I2C_CR1_SWRST; // Clear reset

  // 16 Mhz Frequency P_clk
  I2C1->CR2 = (16U << I2C_CR2_FREQ_Pos);

  // Configure Clock Control (Standard Mode 100kHz)
  // CCR = PCLK / (2 * 100,000) = 16,000,000 / 200,000 = 80
  I2C1->CCR &= ~I2C_CCR_CCR_Msk;
  I2C1->CCR &= ~(I2C_CCR_FS);
  I2C1->CCR = 80;

  // Max Rise Time
  // Standard Mode max rise time = 1000ns.
  // TRISE = (PCLK * 1000ns) + 1 = (16 * 1) + 1 = 17
  I2C1->TRISE = 17;

  I2C1->CR1 |= I2C_CR1_PE; // Enable Peripheral
}

/******************************************************************************
  Function: I2C Send Data [Byte]
******************************************************************************/
int i2c_write_byte(uint8_t saddr, uint8_t data) {
  volatile uint32_t timeout; // Initialize Timeout

  // Wait until the bus is not busy
  timeout = I2C_TIMEOUT_VAL;
  while ((I2C1->SR2 & I2C_SR2_BUSY) && --timeout);
  if (timeout == 0)
    return -1;

  I2C1->CR1 |= I2C_CR1_START; // Generate START condition

  timeout = I2C_TIMEOUT_VAL;                      // Re_Initialize Timeout
  while (!(I2C1->SR1 & I2C_SR1_SB) && --timeout); // Wait for SB flag
  if (timeout == 0)
    return -1;

  I2C1->DR = (saddr << 1); // Send Target Address

  // Wait for ADDR flag or ACK Failure (AF)
  timeout = I2C_TIMEOUT_VAL; // Re_Initialize Timeout
  while (!(I2C1->SR1 & (I2C_SR1_ADDR | I2C_SR1_AF)) && --timeout);

  // Check if we exited because of a NACK (Target didn't answer)
  if ((I2C1->SR1 & I2C_SR1_AF) || (timeout == 0)) {
    I2C1->SR1 &= ~I2C_SR1_AF;  // Clear error flag
    I2C1->CR1 |= I2C_CR1_STOP; // Release the bus
    return -1;                 // Failure Code
  }

  // Clear ADDR flag (Reading SR1 then SR2)
  (void)I2C1->SR1;
  (void)I2C1->SR2;

  // Wait for TXE (Data register empty)
  timeout = I2C_TIMEOUT_VAL; // Re_Initialize Timeout
  while (!(I2C1->SR1 & I2C_SR1_TXE) && --timeout);
  if (timeout == 0)
    return -1; // Failure Code

  // Write Data
  I2C1->DR = data;

  // Wait for BTF (Transfer Finished)
  timeout = I2C_TIMEOUT_VAL; // Re_Initialize Timeout
  while (!(I2C1->SR1 & I2C_SR1_BTF) && --timeout);
  if (timeout == 0)
    return -1; // Failure Code

  // Generate STOP condition
  I2C1->CR1 |= I2C_CR1_STOP;

  GPIOC->ODR ^= (1 << 13); // Toggle Builtin-LED
  return 0;                // Success!
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
  // 1. Initialize the peripheral
  init_i2c();
  init_builtin_led();

  // Example: Send 0x42 to a device with address 0x3C (like an OLED)
  uint8_t slave_addr = 0x3C;
  uint8_t payload = 0x42;

  GPIOC->ODR ^= (1 << 13);
  while (1) {
    i2c_write_byte(slave_addr, payload);

    // Simple delay loop for visibility
    for (volatile int i = 0; i < 100000; i++);
  }
  return 0;
}
