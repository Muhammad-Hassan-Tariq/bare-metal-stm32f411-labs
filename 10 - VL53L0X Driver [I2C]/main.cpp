/**
 * @file    main.cpp
 * @author  [Muhammad Hassan Tariq/ https://github.com/Muhammad-Hassan-Tariq]
 * @brief   Bare-metal implementation VL53L0X Driver
 * @details Works with VL53L0X Sensor on Bare Metal
 * @date    2026-02-13
 * @note    Target: STM32F411CEU6 (Black Pill)
 */

#include "../platform/drivers/stm32f411xe.h"
#include <cstdint>
#include <string>

// BRIDGE: The API is C, so we wrap the headers and the glue functions
extern "C" {
#include "VL53L0X_API/vl53l0x_api.h"
#include "VL53L0X_API/vl53l0x_platform.h"

// These MUST be defined for the platform layer to link
void i2c_write_burst(uint8_t dev_addr, uint8_t start_reg, uint8_t *data, uint32_t length);
void i2c_read_burst(uint8_t dev_addr, uint8_t start_reg, uint8_t *buffer, uint32_t length);
}

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
void sendData(const std::string &data) {
    for (int i = 0; i < data.size(); i++) {
        if (data[i] == '\0')
            break; // Don't send null terminator
        sendByte(data[i]);
    }
}

/******************************************************************************
  Function: Initialize I2C
******************************************************************************/
void init_i2c() {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    __DSB();

    GPIOB->MODER &= ~(GPIO_MODER_MODE6_Msk | GPIO_MODER_MODE7_Msk);
    GPIOB->MODER |= (0b10 << GPIO_MODER_MODE6_Pos) | (0b10 << GPIO_MODER_MODE7_Pos);
    GPIOB->OTYPER |= GPIO_OTYPER_OT6 | GPIO_OTYPER_OT7;
    GPIOB->AFR[0] |= (0b100 << GPIO_AFRL_AFSEL6_Pos) | (0b100 << GPIO_AFRL_AFSEL7_Pos);

    I2C1->CR1 &= ~I2C_CR1_PE;
    I2C1->CR2 = 16;
    I2C1->CCR = 80;
    I2C1->TRISE = 17;
    I2C1->CR1 |= I2C_CR1_PE;
}

/******************************************************************************
  Function: API Functions
******************************************************************************/
// Helper for status polling
bool wait_for_flag(uint32_t bit) {
    uint32_t timeout = 10000;
    while (!(I2C1->SR1 & bit) && --timeout);
    return timeout > 0;
}

// The API calls these
extern "C" void i2c_write_burst(uint8_t dev_addr, uint8_t start_reg, uint8_t *data, uint32_t length) {
    I2C1->CR1 |= I2C_CR1_START;
    if (!wait_for_flag(I2C_SR1_SB)) return;

    I2C1->DR = (dev_addr << 1); // Write
    if (!wait_for_flag(I2C_SR1_ADDR)) return;
    (void)I2C1->SR2;

    I2C1->DR = start_reg;
    if (!wait_for_flag(I2C_SR1_TXE)) return;

    for (uint32_t i = 0; i < length; i++) {
        I2C1->DR = data[i];
        wait_for_flag(I2C_SR1_TXE);
    }
    wait_for_flag(I2C_SR1_BTF);
    I2C1->CR1 |= I2C_CR1_STOP;
}

extern "C" void i2c_read_burst(uint8_t dev_addr, uint8_t start_reg, uint8_t *buffer, uint32_t length) {
    // Send register address
    I2C1->CR1 |= I2C_CR1_START;
    wait_for_flag(I2C_SR1_SB);
    I2C1->DR = (dev_addr << 1);
    wait_for_flag(I2C_SR1_ADDR);
    (void)I2C1->SR2;
    I2C1->DR = start_reg;
    wait_for_flag(I2C_SR1_TXE);

    // Repeated Start for Read
    I2C1->CR1 |= I2C_CR1_START;
    wait_for_flag(I2C_SR1_SB);
    I2C1->DR = (dev_addr << 1) | 1;
    wait_for_flag(I2C_SR1_ADDR);
    (void)I2C1->SR2;

    for (uint32_t i = 0; i < length; i++) {
        if (i == length - 1) I2C1->CR1 &= ~I2C_CR1_ACK; // NACK last byte
        wait_for_flag(I2C_SR1_RXNE);
        buffer[i] = I2C1->DR;
    }
    I2C1->CR1 |= I2C_CR1_STOP;
    I2C1->CR1 |= I2C_CR1_ACK;
}

void delay_ms(uint32_t ms) {
    for (volatile uint32_t i = 0; i < ms * 4000; i++);
}

extern "C" {
// Write Multi-bytes
VL53L0X_Error VL53L0X_WriteMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count) {
    i2c_write_burst(Dev->I2cDevAddr, index, pdata, count);
    return VL53L0X_ERROR_NONE;
}

// Read Multi-bytes
VL53L0X_Error VL53L0X_ReadMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count) {
    i2c_read_burst(Dev->I2cDevAddr, index, pdata, count);
    return VL53L0X_ERROR_NONE;
}

// Single Byte Write/Read
VL53L0X_Error VL53L0X_WrByte(VL53L0X_DEV Dev, uint8_t index, uint8_t data) {
    return VL53L0X_WriteMulti(Dev, index, &data, 1);
}
VL53L0X_Error VL53L0X_RdByte(VL53L0X_DEV Dev, uint8_t index, uint8_t *data) {
    return VL53L0X_ReadMulti(Dev, index, data, 1);
}

// Word (16-bit) and DWord (32-bit) - Note: VL53L0X is Big Endian!
VL53L0X_Error VL53L0X_WrWord(VL53L0X_DEV Dev, uint8_t index, uint16_t data) {
    uint8_t buffer[2] = {(uint8_t)(data >> 8), (uint8_t)(data & 0x00FF)};
    return VL53L0X_WriteMulti(Dev, index, buffer, 2);
}
VL53L0X_Error VL53L0X_WrDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t data) {
    uint8_t buffer[4] = {(uint8_t)(data >> 24), (uint8_t)(data >> 16), (uint8_t)(data >> 8), (uint8_t)(data & 0x00FF)};
    return VL53L0X_WriteMulti(Dev, index, buffer, 4);
}
VL53L0X_Error VL53L0X_RdWord(VL53L0X_DEV Dev, uint8_t index, uint16_t *data) {
    uint8_t buffer[2];
    VL53L0X_Error status = VL53L0X_ReadMulti(Dev, index, buffer, 2);
    *data = ((uint16_t)buffer[0] << 8) | (uint16_t)buffer[1];
    return status;
}
VL53L0X_Error VL53L0X_RdDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t *data) {
    uint8_t buffer[4];
    VL53L0X_Error status = VL53L0X_ReadMulti(Dev, index, buffer, 4);
    *data = ((uint32_t)buffer[0] << 24) | ((uint32_t)buffer[1] << 16) | ((uint32_t)buffer[2] << 8) | (uint32_t)buffer[3];
    return status;
}

// Delay and Polling
VL53L0X_Error VL53L0X_PollingDelay(VL53L0X_DEV Dev) {
    delay_ms(1); // Replace with your actual delay function
    return VL53L0X_ERROR_NONE;
}

// Update Byte (Read-Modify-Write)
VL53L0X_Error VL53L0X_UpdateByte(VL53L0X_DEV Dev, uint8_t index, uint8_t AndData, uint8_t OrData) {
    uint8_t data;
    VL53L0X_Error status = VL53L0X_RdByte(Dev, index, &data);
    if (status != VL53L0X_ERROR_NONE) return status;
    data = (data & AndData) | OrData;
    return VL53L0X_WrByte(Dev, index, data);
}
}

/******************************************************************************
  Function: Buzzer & LED Initialize
******************************************************************************/
void init_buzzer_led() {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    GPIOA->MODER &= ~GPIO_MODER_MODE6_Msk;
    GPIOA->MODER &= ~GPIO_MODER_MODE7_Msk;
    GPIOA->MODER |= (0b1 << GPIO_MODER_MODE6_Pos);
    GPIOA->MODER |= (0b1 << GPIO_MODER_MODE7_Pos);
}

/******************************************************************************
  Function: Main Function
******************************************************************************/
int main() {
    init_i2c();
    init_uart();
    init_buzzer_led();

    VL53L0X_Dev_t MyDevice;
    MyDevice.I2cDevAddr = 0x29; // Default 7-bit addr

    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    VL53L0X_RangingMeasurementData_t RangingData;

    // --- THE "ANNOYING" API SEQUENCE ---
    Status = VL53L0X_DataInit(&MyDevice);
    Status = VL53L0X_StaticInit(&MyDevice);
    Status = VL53L0X_PerformRefCalibration(&MyDevice, NULL, NULL);
    Status = VL53L0X_PerformRefSpadManagement(&MyDevice, NULL, NULL);
    Status = VL53L0X_SetDeviceMode(&MyDevice, VL53L0X_DEVICEMODE_SINGLE_RANGING);

    while (1) {
        Status = VL53L0X_PerformSingleRangingMeasurement(&MyDevice, &RangingData);
        if (Status == VL53L0X_ERROR_NONE) {
            uint16_t distance = RangingData.RangeMilliMeter;
            // 'distance' contains bare-metal reading!
            sendData("Distance: ");
            sendData(std::to_string(distance));
            sendData(" mm\r\n");

            if (distance <= 70) {
                GPIOA->ODR |= GPIO_ODR_OD6;
                GPIOA->ODR |= GPIO_ODR_OD7;
            } else {
                GPIOA->ODR &= ~GPIO_ODR_OD6_Msk;
                GPIOA->ODR &= ~GPIO_ODR_OD7_Msk;
            }
        }
        for (volatile int i = 0; i < 500; i++); // Simple delay
    }
}
