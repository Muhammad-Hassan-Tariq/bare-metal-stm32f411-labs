/**
 * @file    main.cpp
 * @author  [Muhammad Hassan Tariq/ https://github.com/Muhammad-Hassan-Tariq]
 * @brief   Bare-metal implementation of I2C Driver
 * @details Implemented Bare Metal I2C driver with polling. Read/ Write Single/ Multiple Bytes
 * @date    2026-02-11
 * @note    Target: STM32F411CEU6 (Black Pill)
 */

#include "../platform/drivers/stm32f411xe.h"
#include <cstdint>

/******************************************************************************
  Function: Initialize I2C
******************************************************************************/
void init_i2c() {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    __DSB();

    // GPIO Configurations
    GPIOB->MODER &= ~(GPIO_MODER_MODE6_Msk | GPIO_MODER_MODE7_Msk);
    GPIOB->MODER |= (0b10 << GPIO_MODER_MODE6_Pos) | (0b10 << GPIO_MODER_MODE7_Pos);
    GPIOB->OTYPER |= GPIO_OTYPER_OT6 | GPIO_OTYPER_OT7;
    GPIOB->AFR[0] &= ~(GPIO_AFRL_AFSEL6_Msk | GPIO_AFRL_AFSEL7_Msk);
    GPIOB->AFR[0] |= (0b100 << GPIO_AFRL_AFSEL6_Pos) | (0b100 << GPIO_AFRL_AFSEL7_Pos);

    // I2C Configurations
    I2C1->CR1 &= ~I2C_CR1_PE;
    I2C1->CR1 |= I2C_CR1_SWRST;
    I2C1->CR1 &= ~I2C_CR1_SWRST;

    I2C1->CR2 = (16U << I2C_CR2_FREQ_Pos);
    I2C1->CCR &= ~I2C_CCR_CCR_Msk;
    I2C1->CCR &= ~(I2C_CCR_FS);
    I2C1->CCR = 80;
    I2C1->TRISE = 17;

    I2C1->CR1 |= I2C_CR1_PE; // Enable I2C
}

/******************************************************************************
  Enums/ Functions/ Structs & Constants for I2C
******************************************************************************/
static constexpr uint16_t I2C_timeout_VAL = 500;

enum I2C_Flags {
    SB,
    ADDR,
    TXE,
    BTF,
    RXNE
};
struct I2C_Functions {
    static void i2c_start_condtition() { I2C1->CR1 |= I2C_CR1_START; }
    static void i2c_send_dev_addr(uint8_t dev_addr, bool writeBit) { I2C1->DR = (dev_addr << 1) | ((writeBit) ? 0 : 1); }
    static void i2c_clear_status_register() {
        (void)I2C1->SR1;
        (void)I2C1->SR2;
    }
    static void i2c_send_data(uint8_t data) { I2C1->DR = data; }
    static void i2c_stop_condition() { I2C1->CR1 |= I2C_CR1_STOP; }
    static void i2c_enable_ack(bool en_dis_ack) { I2C1->CR1 |= I2C_CR1_ACK; }
    static void i2c_disable_ack(bool en_dis_ack) { I2C1->CR1 &= ~I2C_CR1_ACK; }

    static bool i2c_timeout_delay(I2C_Flags flag_check) {
        uint16_t timeout = I2C_timeout_VAL;
        switch (flag_check) {
        case SB:
            while (!(I2C1->SR1 & I2C_SR1_SB) && --timeout);
            break;
        case ADDR:
            while (!(I2C1->SR1 & I2C_SR1_ADDR) && --timeout);
            break;
        case TXE:
            while (!(I2C1->SR1 & I2C_SR1_TXE) && --timeout);
            break;
        case BTF:
            while (!(I2C1->SR1 & I2C_SR1_BTF) && --timeout);
            break;
        case RXNE:
            while (!(I2C1->SR1 & I2C_SR1_RXNE) && --timeout);
            break;
        }
        return (timeout == 0) ? false : true;
    }
};

/******************************************************************************
  Function: Write Single Byte
******************************************************************************/
void i2c_write_single_byte(uint8_t dev_addr, uint8_t reg_addr, uint8_t data) {
    I2C_Functions::i2c_start_condtition();
    if (!I2C_Functions::i2c_timeout_delay(SB))
        return;

    I2C_Functions::i2c_send_dev_addr(dev_addr, true);
    if (!I2C_Functions::i2c_timeout_delay(ADDR))
        return;
    I2C_Functions::i2c_clear_status_register();

    I2C_Functions::i2c_send_data(reg_addr);
    if (!I2C_Functions::i2c_timeout_delay(TXE))
        return;

    I2C_Functions::i2c_send_data(data);
    if (!I2C_Functions::i2c_timeout_delay(BTF))
        return;
    I2C_Functions::i2c_stop_condition();
}

/******************************************************************************
  Function: Write Multiple Bytes
******************************************************************************/
void i2c_write_burst(uint8_t dev_addr, uint8_t start_reg, uint8_t *data, uint32_t length) {
    I2C_Functions::i2c_start_condtition();
    if (!I2C_Functions::i2c_timeout_delay(SB))
        return;

    I2C_Functions::i2c_send_dev_addr(dev_addr, true);
    if (!I2C_Functions::i2c_timeout_delay(ADDR))
        return;
    I2C_Functions::i2c_clear_status_register();

    I2C_Functions::i2c_send_data(start_reg);
    if (!I2C_Functions::i2c_timeout_delay(TXE))
        return;

    for (uint32_t i = 0; i < length; i++) {
        I2C_Functions::i2c_send_data(data[i]);
        if (!I2C_Functions::i2c_timeout_delay(TXE))
            return;
    }

    if (!I2C_Functions::i2c_timeout_delay(BTF))
        return;
    I2C_Functions::i2c_stop_condition();
}

/******************************************************************************
  Function: Read Single Byte
******************************************************************************/
uint8_t i2c_read_single_byte(uint8_t dev_addr, uint8_t reg_addr) {
    uint8_t data = 0;

    // Phase 1
    I2C_Functions::i2c_start_condtition();
    if (!I2C_Functions::i2c_timeout_delay(SB))
        return 0;

    I2C_Functions::i2c_send_dev_addr(dev_addr, true);
    if (!I2C_Functions::i2c_timeout_delay(ADDR))
        return 0;

    I2C_Functions::i2c_clear_status_register();
    I2C_Functions::i2c_send_data(reg_addr);
    if (!I2C_Functions::i2c_timeout_delay(TXE))
        return 0;

    // Phase 2
    I2C_Functions::i2c_start_condtition();
    if (!I2C_Functions::i2c_timeout_delay(SB))
        return 0;

    I2C_Functions::i2c_send_dev_addr(dev_addr, false);
    I2C_Functions::i2c_disable_ack(false);

    if (!I2C_Functions::i2c_timeout_delay(ADDR))
        return 0;
    I2C_Functions::i2c_clear_status_register();
    I2C_Functions::i2c_stop_condition();

    if (!I2C_Functions::i2c_timeout_delay(RXNE))
        return 0;

    data = I2C1->DR;
    I2C_Functions::i2c_enable_ack(true);

    return data;
}

/******************************************************************************
  Function: Read Multiple Bytes
******************************************************************************/
void i2c_read_burst(uint8_t dev_addr, uint8_t start_reg, uint8_t *buffer, uint32_t length) {
    // PHASE 1: Set the register pointer (Write Mode)
    I2C_Functions::i2c_start_condtition();
    if (!I2C_Functions::i2c_timeout_delay(SB))
        return;

    I2C_Functions::i2c_send_dev_addr(dev_addr, true);
    if (!I2C_Functions::i2c_timeout_delay(ADDR))
        return;

    I2C_Functions::i2c_clear_status_register();
    I2C_Functions::i2c_send_data(start_reg);
    if (!I2C_Functions::i2c_timeout_delay(BTF))
        return;

    // PHASE 2: Receive Data (Read Mode)
    I2C_Functions::i2c_start_condtition(); // Repeated Start
    if (!I2C_Functions::i2c_timeout_delay(SB))
        return;

    I2C_Functions::i2c_send_dev_addr(dev_addr, false); // Read mode
    if (!I2C_Functions::i2c_timeout_delay(ADDR))
        return;

    // Enable ACKing so sensor keeps sending
    I2C_Functions::i2c_enable_ack(true);
    I2C_Functions::i2c_clear_status_register();

    for (uint32_t i = 0; i < length; i++) {
        if (i == (length - 1)) {
            // THE LAST BYTE: We must NACK and STOP before reading DR
            I2C_Functions::i2c_disable_ack(false);
            I2C_Functions::i2c_stop_condition();
        }

        // Wait for byte to arrive
        if (!I2C_Functions::i2c_timeout_delay(RXNE))
            return;
        buffer[i] = I2C1->DR;
    }

    // Re-enable ACK for the next transaction
    I2C_Functions::i2c_enable_ack(true);
}

/******************************************************************************
  Function: Main Function
******************************************************************************/
int main() {
    init_i2c();
    while (true) {
        i2c_write_single_byte(0x76, 0xD0, 10);
    }
}
