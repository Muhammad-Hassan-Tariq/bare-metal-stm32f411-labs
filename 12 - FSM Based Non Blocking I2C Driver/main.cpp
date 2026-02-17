/**
 * @file    main.cpp
 * @author  [Muhammad Hassan Tariq/ https://github.com/Muhammad-Hassan-Tariq]
 * @brief   FSM based Non Blocking I2C Driver
 * @details Implemented an I2C FSM based Non Blocking Driver
 * @notice  Commenting was AI Assisted
 * @date    2026-02-17
 * @note    Target: STM32F411CEU6 (Black Pill)
 */

#include "../platform/drivers/stm32f411xe.h"
#include "./i2c_driver.cpp"
#include <cstdint>

// 1. Declare the tick variable FIRST so the namespace can see it
volatile uint32_t ms_ticks = 0;

// 2. Define the tick provider for the I2C_DRIVER namespace
namespace I2C_DRIVER {
    uint32_t i2c_get_tick_ms(void) {
        return ms_ticks;
    }
} // namespace I2C_DRIVER

using namespace I2C_DRIVER;

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================
I2C_FSM i2c1_fsm;
uint8_t tx_data[] = {0xDE, 0xAD, 0xBE, 0xEF};
uint8_t rx_buffer[10];
volatile bool transaction_complete = false;
volatile I2C_Status last_status = I2C_OK;

// ============================================================================
// CALLBACK FUNCTION
// ============================================================================
void my_i2c_callback(I2C_Status status, void* user_data) {
    last_status = status;
    transaction_complete = true;
}

// ============================================================================
// SYSTEM TICK & ISR ROUTING
// ============================================================================
extern "C" {
// CMSIS Standard Handler
void SysTick_Handler(void) {
    ms_ticks++;
}

void I2C1_EV_IRQHandler(void) {
    i2c_handle_event_irq(&i2c1_fsm, I2C1);
}

void I2C1_ER_IRQHandler(void) {
    i2c_handle_error_irq(&i2c1_fsm, I2C1);
}
}

// ============================================================================
// MAIN
// ============================================================================
int main() {
    // 1. Clock and SysTick Setup
    // Default HSI is 16MHz. SysTick_Config takes number of ticks between interrupts.
    // 16,000,000 / 1000 = 16,000 ticks per 1ms.
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000);

    uint32_t pclk1_hz = 16000000;

    // 2. Initialize GPIO C13 (LED)
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    GPIOC->MODER &= ~GPIO_MODER_MODE13_Msk;
    GPIOC->MODER |= (0b01 << GPIO_MODER_MODE13_Pos);
    GPIOC->ODR |= GPIO_ODR_OD13;

    // 3. Initialize I2C Peripheral (100kHz)
    i2c_init(I2C1, 100, pclk1_hz);

    while (true) {
        if (i2c1_fsm.current_state == I2C_STATE_IDLE) {
            transaction_complete = false;

            // Start Async Write: Address 0x3C, 4 bytes
            i2c_master_write_async(I2C1, &i2c1_fsm, 0x3C, tx_data, 4, 100, my_i2c_callback);
        }

        // 4. Wait for completion (LED will toggle only on status change)
        while (!transaction_complete) {
            __asm__ volatile("nop");
        }

        if (last_status == I2C_OK) {
            GPIOC->ODR ^= GPIO_ODR_OD13;
        }

        // Simple loop delay
        for (volatile int i = 0; i < 500000; i++);
    }
}
