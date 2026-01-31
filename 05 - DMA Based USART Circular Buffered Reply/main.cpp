/**
 * @file    main.cpp
 * @author  [Muhammad Hassan Tariq/ https://github.com/Muhammad-Hassan-Tariq]
 * @brief   Bare-metal implementation of USART RX, TX in Circular Mode
 * @details Implemented DMA based USART RX reply with TX using buffer. Data recieved On
            RX is buffered and released when buffer reaches it's capacity. In our case
            it's 10.
            // Tabular Form
            PIN A2         TX | Alternate Function [AF7]
            PIN A3         RX | Alternate Function [AF7]
            USART2         AF7, RX & TX
 * @date    2026-01-31
 * @note    Target: STM32F411CEU6 (Black Pill)
 */

#include "../platform/drivers/stm32f411xe.h"
#include <cstdint>

volatile bool buffer_full = false; // Buffer flag
volatile char buffer[10] = {"000000000"};

/******************************************************************************
  Function: Initialize USART2
******************************************************************************/
void init_uart() {
  // Enable Clocks USART2 & GPIOA
  RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

  // Clear & Set Mode to Alternate Function PA2[TX] & PA3[RX]
  GPIOA->MODER &= ~(GPIO_MODER_MODER2_Msk | GPIO_MODER_MODER3_Msk);
  GPIOA->MODER |= (0b10 << GPIO_MODER_MODER2_Pos | 0b10 << GPIO_MODER_MODER3_Pos);

  // Clear & Set AF7 to PA2 & PA3
  GPIOA->AFR[0] &= ~(GPIO_AFRL_AFSEL2_Msk | GPIO_AFRL_AFSEL3_Msk);
  GPIOA->AFR[0] |= (7U << GPIO_AFRL_AFSEL2_Pos | 7U << GPIO_AFRL_AFSEL3_Pos);

  // Set Baud Rate for HSI 16 Mhz 115200 Baud Rate
  // BRR = 16,000,000 / (16 * 115200) -> 8.680
  // Mantissa = 8  |  Fraction = 0.680 * 16 -> 11
  USART2->BRR = (8U << USART_BRR_DIV_Mantissa_Pos) | (11U << USART_BRR_DIV_Fraction_Pos);

  USART2->CR3 |= USART_CR3_DMAR;                            // Enable DMA on RX
  USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE; // Enable TX, RX & UART
}

/******************************************************************************
  Function: Initialize DMA
******************************************************************************/
void init_dma() {
  RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN; // Enable DMA1 Clock

  DMA1_Stream5->CR &= ~DMA_SxCR_EN;       // Ensure Stream is disabled before configuration
  while (DMA1_Stream5->CR & DMA_SxCR_EN); // Wait for stream to stop

  // Clear & Set bits for Channel 4 Selection
  DMA1_Stream5->CR &= ~(DMA_SxCR_CHSEL_Msk);
  DMA1_Stream5->CR |= (0b100 << DMA_SxCR_CHSEL_Pos);

  DMA1_Stream5->CR |= DMA_SxCR_MINC; // Memory Increment Mode
  DMA1_Stream5->CR |= DMA_SxCR_CIRC; // Circular Mode
  DMA1_Stream5->CR |= DMA_SxCR_TCIE; // Transfer Complete Interrut

  DMA1_Stream5->PAR = (uintptr_t)&USART2->DR; // Peripheral Address
  DMA1_Stream5->M0AR = (uintptr_t)&buffer;    // Memory Address
  DMA1_Stream5->NDTR = 10U;                   // No of transfers

  DMA1_Stream5->CR |= DMA_SxCR_EN; // Enable DMA
  NVIC_EnableIRQ(DMA1_Stream5_IRQn);
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
void sendData(const volatile char (*data)[10]) {
  for (int i = 0; i < 10; ++i) {
    if ((*data)[i] == '\0') {
      break; // Break if null terminator
    }
    sendByte((*data)[i]);
  }

  // Send New Line Sequence
  sendByte('\r');
  sendByte('\n');
}

/******************************************************************************
  Function: DMA Interrupt Handler
******************************************************************************/
extern "C" void DMA1_Stream5_IRQHandler() {
  if (DMA1->HISR & DMA_HISR_TCIF5) {
    DMA1->HIFCR = DMA_HIFCR_CTCIF5; // Clear Flag
    buffer_full = true;             // Buffer Full
  }
}

/******************************************************************************
  Function: Main Function
******************************************************************************/
int main() {
  init_uart(); // Init UART
  init_dma();  // Init DMA

  while (1) {
    __WFI(); // Wait unless interrupted
    if (buffer_full) {
      sendData(&buffer);
      for (int i = 0; i < 10; i++)
        buffer[i] = '0';

      buffer_full = false;
    }
  }
}
