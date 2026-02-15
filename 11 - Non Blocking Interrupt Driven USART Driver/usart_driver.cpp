/**
 * @file    usart_driver.cpp
 * @author  [Muhammad Hassan Tariq/ https://github.com/Muhammad-Hassan-Tariq]
 * @brief   Non Blocking Interrupt Driven USART DRIVER
 * @details Implemented a Ring Buffered Non Blocking Interrupt Driven USART DRIVER
 * @date    2026-02-15
 * @note    Target: STM32F411CEU6 (Black Pill)
 *
 * @license GNU General Public License v3.0
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#pragma once
#ifndef USART_DRIVER_H
#define USART_DRIVER_H

/******************************************************************************
  Namespace: USART DRIVER
******************************************************************************/
namespace USART_DRIVER {
    struct USART_VARIABLES {
            static const int BUF_SIZE = 256;
            static const int MASK = BUF_SIZE - 1;

            volatile uint8_t tx_buffer[BUF_SIZE];
            volatile uint32_t tx_head = 0;
            volatile uint32_t tx_tail = 0;

            volatile uint8_t rx_buffer[BUF_SIZE];
            volatile uint32_t rx_head = 0;
            volatile uint32_t rx_tail = 0;
    };

    // Data Structure Objects
    USART_VARIABLES USART1_DATA;
    USART_VARIABLES USART2_DATA;
    USART_VARIABLES USART6_DATA;

    inline void send_async(USART_TypeDef* USART, const char* str); // SEND IN QUEUE
    inline bool available(USART_TypeDef* USART);                   // DATA AVAILABLE
    inline uint8_t read(USART_TypeDef* USART);                     // READ DATA

    inline void init_uart(USART_TypeDef* USART, const int& baud, const int& clock); // INITIALIZE UART
} // namespace USART_DRIVER

inline void USART_DRIVER::send_async(USART_TypeDef* USART, const char* str) {
    USART_VARIABLES* data = (USART == USART1) ? &USART1_DATA : (USART == USART2) ? &USART2_DATA
                                                                                 : &USART6_DATA;

    while (*str) {
        uint32_t next = (data->tx_head + 1) & USART_VARIABLES::MASK;
        while (next == data->tx_tail);
        data->tx_buffer[data->tx_head] = *str++;
        data->tx_head = next;
    }
    USART->CR1 |= USART_CR1_TXEIE;
}

inline bool USART_DRIVER::available(USART_TypeDef* USART) {
    USART_VARIABLES* data = (USART == USART1) ? &USART1_DATA : (USART == USART2) ? &USART2_DATA
                                                                                 : &USART6_DATA;
    return data->rx_head != data->rx_tail;
}

inline uint8_t USART_DRIVER::read(USART_TypeDef* USART) {
    USART_VARIABLES* data = (USART == USART1) ? &USART1_DATA : (USART == USART2) ? &USART2_DATA
                                                                                 : &USART6_DATA;
    if (data->rx_head == data->rx_tail) return 0;
    uint8_t byte = data->rx_buffer[data->rx_tail];
    data->rx_tail = (data->rx_tail + 1) & USART_DRIVER::USART_VARIABLES::MASK;
    return byte;
}

inline void USART_DRIVER::init_uart(USART_TypeDef* USART, const int& baud, const int& clock) {
    // 1. Clock Enable & Pin Config
    if (USART == USART2) {
        RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
        GPIOA->MODER |= (0b10 << GPIO_MODER_MODER2_Pos) | (0b10 << GPIO_MODER_MODER3_Pos);
        GPIOA->AFR[0] |= (7U << GPIO_AFRL_AFSEL2_Pos) | (7U << GPIO_AFRL_AFSEL3_Pos);
    } else if (USART == USART1) {
        RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
        GPIOA->MODER |= (0b10 << GPIO_MODER_MODER9_Pos) | (0b10 << GPIO_MODER_MODER10_Pos);
        GPIOA->AFR[1] |= (7U << GPIO_AFRH_AFSEL9_Pos) | (7U << GPIO_AFRH_AFSEL10_Pos);
    } else if (USART == USART6) {
        RCC->APB2ENR |= RCC_APB2ENR_USART6EN;
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
        GPIOA->MODER |= (0b10 << GPIO_MODER_MODER11_Pos) | (0b10 << GPIO_MODER_MODER12_Pos);
        GPIOA->AFR[1] |= (8U << GPIO_AFRH_AFSEL11_Pos) | (8U << GPIO_AFRH_AFSEL12_Pos);
    }

    // 2. BAUD RATE
    double brr_calc = (double)clock / (16.0 * baud);
    int mantissa = int(brr_calc);
    int fraction = int((brr_calc - mantissa) * 16.0 + 0.5);

    if (fraction >= 16) {
        mantissa += 1;
        fraction = 0;
    }

    USART->BRR = (mantissa << 4) | (fraction & 0x0F);

    // 3. Enable Bits [Transmission, Reception, Enable, RXN Interrupt, IDLE Interrupt]
    USART->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE | USART_CR1_RXNEIE | USART_CR1_IDLEIE;

    // 4. Enable NVIC
    if (USART == USART1) NVIC_EnableIRQ(USART1_IRQn);
    else if (USART == USART2) NVIC_EnableIRQ(USART2_IRQn);
    else if (USART == USART6) NVIC_EnableIRQ(USART6_IRQn);
}

/******************************************************************************
  Function: USART1 IRQ_HANDLER
******************************************************************************/
extern "C" void USART1_IRQHandler() {
    uint32_t sr = USART1->SR;
    auto& data_struct = USART_DRIVER::USART1_DATA;

    // Critical: Clear errors before processing data
    if (sr & (USART_SR_ORE | USART_SR_FE | USART_SR_PE)) {
        volatile uint32_t dummy = USART1->DR;
        return;
    }

    // RX
    if (sr & USART_SR_RXNE) {
        uint8_t data = (uint8_t)USART1->DR;
        uint32_t next = (data_struct.rx_head + 1) & USART_DRIVER::USART_VARIABLES::MASK;
        if (next != data_struct.rx_tail) {
            data_struct.rx_buffer[data_struct.rx_head] = data;
            data_struct.rx_head = next;
        }
    }

    // IDLE
    if (sr & USART_SR_IDLE) {
        volatile uint32_t dummy = USART1->DR;
    }

    // TX
    if ((USART1->CR1 & USART_CR1_TXEIE) && (sr & USART_SR_TXE)) {
        if (data_struct.tx_head != data_struct.tx_tail) {
            USART1->DR = data_struct.tx_buffer[data_struct.tx_tail];
            data_struct.tx_tail = (data_struct.tx_tail + 1) & USART_DRIVER::USART_VARIABLES::MASK;
        } else {
            USART1->CR1 &= ~USART_CR1_TXEIE;
        }
    }
}

/******************************************************************************
  Function: USART2 IRQ_HANDLER
******************************************************************************/
extern "C" void USART2_IRQHandler() {
    uint32_t sr = USART2->SR;
    auto& data_struct = USART_DRIVER::USART2_DATA;

    if (sr & (USART_SR_ORE | USART_SR_FE | USART_SR_PE)) {
        volatile uint32_t dummy = USART2->DR;
        return;
    }

    if (sr & USART_SR_RXNE) {
        uint8_t data = (uint8_t)USART2->DR;
        uint32_t next = (data_struct.rx_head + 1) & USART_DRIVER::USART_VARIABLES::MASK;
        if (next != data_struct.rx_tail) {
            data_struct.rx_buffer[data_struct.rx_head] = data;
            data_struct.rx_head = next;
        }
    }

    if (sr & USART_SR_IDLE) {
        volatile uint32_t dummy = USART2->DR;
    }

    if ((USART2->CR1 & USART_CR1_TXEIE) && (sr & USART_SR_TXE)) {
        if (data_struct.tx_head != data_struct.tx_tail) {
            USART2->DR = data_struct.tx_buffer[data_struct.tx_tail];
            data_struct.tx_tail = (data_struct.tx_tail + 1) & USART_DRIVER::USART_VARIABLES::MASK;
        } else {
            USART2->CR1 &= ~USART_CR1_TXEIE;
        }
    }
}

/******************************************************************************
  Function: USART6 IRQ_HANDLER
******************************************************************************/
extern "C" void USART6_IRQHandler() {
    uint32_t sr = USART6->SR;
    auto& data_struct = USART_DRIVER::USART6_DATA;

    if (sr & (USART_SR_ORE | USART_SR_FE | USART_SR_PE)) {
        volatile uint32_t dummy = USART6->DR;
        return;
    }

    if (sr & USART_SR_RXNE) {
        uint8_t data = (uint8_t)USART6->DR;
        uint32_t next = (data_struct.rx_head + 1) & USART_DRIVER::USART_VARIABLES::MASK;
        if (next != data_struct.rx_tail) {
            data_struct.rx_buffer[data_struct.rx_head] = data;
            data_struct.rx_head = next;
        }
    }

    if (sr & USART_SR_IDLE) {
        volatile uint32_t dummy = USART6->DR;
    }

    if ((USART6->CR1 & USART_CR1_TXEIE) && (sr & USART_SR_TXE)) {
        if (data_struct.tx_head != data_struct.tx_tail) {
            USART6->DR = data_struct.tx_buffer[data_struct.tx_tail];
            data_struct.tx_tail = (data_struct.tx_tail + 1) & USART_DRIVER::USART_VARIABLES::MASK;
        } else {
            USART6->CR1 &= ~USART_CR1_TXEIE;
        }
    }
}

#endif
