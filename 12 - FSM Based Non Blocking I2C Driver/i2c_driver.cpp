
/**
 * @file    i2c_driver.cpp
 * @author  [Muhammad Hassan Tariq/ https://github.com/Muhammad-Hassan-Tariq]
 * @brief   FSM based Non Blocking I2C Driver
 * @details Implemented an I2C FSM based Non Blocking Driver
 * @date    2026-02-17
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

#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include <cstdint>

namespace I2C_DRIVER {

    // ============================================================================
    // TYPE DEFINITIONS
    // ============================================================================

    /** 📊 I2C FSM State Definitions (6 states, not 10) */
    typedef enum {
        I2C_STATE_IDLE = 0, // Ready for new transaction
        I2C_STATE_WRITE,    // Master TX phase (send addr + register)
        I2C_STATE_RESTART,  // Repeated START condition for read
        I2C_STATE_READ,     // Master RX phase
        I2C_STATE_STOP,     // Cleanup after transaction
        I2C_STATE_ERROR     // Error recovery state
    } I2C_State;

    /** 📋 I2C Transaction Status Codes */
    typedef enum {
        I2C_OK = 0,
        I2C_ERR_BUSY,    // Previous transaction in progress
        I2C_ERR_PARAM,   // Invalid parameter (NULL ptr, size 0)
        I2C_ERR_ADDR,    // Invalid slave address
        I2C_ERR_TIMEOUT, // Transaction timeout (watchdog failure)
        I2C_ERR_NACK,    // Slave NACK received
        I2C_ERR_BUS,     // Bus error (BERR flag)
        I2C_ERR_UNKNOWN  // Unhandled error state
    } I2C_Status;

    /** 🎯 Callback Function Type (Called when transaction completes) */
    typedef void (*I2C_CompletionCallback)(I2C_Status status, void* user_data);

    /** 🔧 FSM Context Structure (Encapsulates transaction state) */
    typedef struct {
            // State machine control
            volatile I2C_State current_state;
            I2C_Status last_error;
            uint8_t retry_count;

            // Slave device addressing
            uint8_t dev_addr; // 7-bit slave address

            // TX Phase (Write Register Address)
            uint8_t* tx_buffer;
            uint16_t tx_bytes;
            volatile uint16_t tx_index;

            // RX Phase (Read Data)
            uint8_t* rx_buffer;
            uint16_t rx_bytes;
            volatile uint16_t rx_index;

            // ⏱️ Timeout Protection (CRITICAL for production)
            uint32_t timeout_ms; // Max transaction time
            uint32_t start_tick; // Saved system tick

            // 📞 Completion Callback
            I2C_CompletionCallback callback;
            void* callback_data;

    } I2C_FSM;

// ============================================================================
// CONFIGURATION CONSTANTS
// ============================================================================

// Default timeout: 100ms (adjust per I2C speed and data size)
#define I2C_DEFAULT_TIMEOUT_MS 100

// Maximum I2C transaction size (safety limit)
#define I2C_MAX_BUFFER_SIZE 256

// Valid 7-bit I2C address range
#define I2C_ADDR_MIN 0x08
#define I2C_ADDR_MAX 0x77

// Bus recovery: number of SCL clock pulses
#define I2C_RECOVERY_PULSES 10

    // ============================================================================
    // HELPER INLINE FUNCTIONS (Minimal, meaningful wrappers)
    // ============================================================================

    /** 🚀 Trigger I2C START Condition */
    static inline void i2c_start(I2C_TypeDef* I2C) { I2C->CR1 |= I2C_CR1_START; }
    static inline void i2c_stop(I2C_TypeDef* I2C) { I2C->CR1 |= I2C_CR1_STOP; }
    static inline void i2c_send_addr(I2C_TypeDef* I2C, uint8_t addr, bool is_write) { I2C->DR = (addr << 1) | (is_write ? 0 : 1); }
    static inline void i2c_send_byte(I2C_TypeDef* I2C, uint8_t data) { I2C->DR = data; }
    static inline uint8_t i2c_read_byte(I2C_TypeDef* I2C) { return I2C->DR; }
    static inline void i2c_ack(I2C_TypeDef* I2C) { I2C->CR1 |= I2C_CR1_ACK; }
    static inline void i2c_nack(I2C_TypeDef* I2C) { I2C->CR1 &= ~I2C_CR1_ACK; }
    static inline void i2c_clear_addr(I2C_TypeDef* I2C) {
        volatile uint32_t sr1 = I2C->SR1;
        volatile uint32_t sr2 = I2C->SR2;
        (void)(sr1);
        (void)(sr2); // Prevent compiler optimization
    }
    static inline bool i2c_is_busy(I2C_TypeDef* I2C) { return (I2C->SR2 & I2C_SR2_BUSY) != 0; }

    /** ⏰ Get System Tick (Implement based on your RTOS/HAL) */
    extern uint32_t i2c_get_tick_ms(void);

    // ============================================================================
    // CORE DRIVER FUNCTIONS
    // ============================================================================

    /**
     * Initialize I2C Peripheral with Clock Configuration
     * @param I2C      Peripheral pointer (I2C1, I2C2, I2C3)
     * @param speed_kHz I2C speed (100 or 400)
     * @param pclk1_hz APB1 clock frequency
     */
    void i2c_init(I2C_TypeDef* I2C, uint16_t speed_kHz, uint32_t pclk1_hz);

    /**
     * Start Interrupt-Driven Write Transaction
     * @param I2C      Peripheral pointer
     * @param fsm      FSM context
     * @param addr     7-bit slave address
     * @param data     Data to write
     * @param size     Number of bytes
     * @param timeout  Transaction timeout in ms
     * @param callback Completion callback (NULL if polling)
     * @return I2C_OK on success, error code otherwise
     */
    I2C_Status i2c_master_write_async(I2C_TypeDef* I2C, I2C_FSM* fsm, uint8_t addr, uint8_t* data, uint16_t size, uint32_t timeout, I2C_CompletionCallback callback);

    /**
     * Start Interrupt-Driven Write-then-Read Transaction
     * @param I2C      Peripheral pointer
     * @param fsm      FSM context
     * @param addr     7-bit slave address
     * @param reg      Register/command buffer to write
     * @param reg_size Bytes to write
     * @param rx_buf   Buffer for read data
     * @param rx_size  Bytes to read
     * @param timeout  Transaction timeout in ms
     * @param callback Completion callback
     * @return I2C_OK on success, error code otherwise
     */
    I2C_Status i2c_master_read_async(I2C_TypeDef* I2C, I2C_FSM* fsm, uint8_t addr, uint8_t* reg, uint16_t reg_size, uint8_t* rx_buf, uint16_t rx_size, uint32_t timeout, I2C_CompletionCallback callback);

    /**
     * FSM State Machine Processor (Called from ISR)
     * @param fsm FSM context
     * @param I2C Peripheral pointer
     */
    void i2c_fsm_process(I2C_FSM* fsm, I2C_TypeDef* I2C);

    /**
     * Bus Recovery via GPIO Bit-Banging
     * Implements Zhiyao recovery pattern: 10 SCL pulses with SDA held high
     * @param I2C        Peripheral pointer
     * @param scl_port   GPIO port for SCL
     * @param scl_pin    GPIO pin for SCL
     * @param sda_port   GPIO port for SDA
     * @param sda_pin    GPIO pin for SDA
     */
    void i2c_bus_recovery(I2C_TypeDef* I2C, GPIO_TypeDef* scl_port, uint16_t scl_pin, GPIO_TypeDef* sda_port, uint16_t sda_pin);

    /**
     * Software Reset I2C Peripheral
     * Clears PE bit to reset internal state machines
     * @param I2C Peripheral pointer
     */
    void i2c_software_reset(I2C_TypeDef* I2C);

    /**
     * Get Human-Readable Error String
     * @param status Error code
     * @return Const string description
     */
    const char* i2c_status_str(I2C_Status status);

    // ============================================================================
    // ISR HANDLER DECLARATIONS (Call from your interrupt handlers)
    // ============================================================================

    void i2c_handle_event_irq(I2C_FSM* fsm, I2C_TypeDef* I2C); /** Event Interrupt Handler */
    void i2c_handle_error_irq(I2C_FSM* fsm, I2C_TypeDef* I2C); /** Error Interrupt Handler */

} // namespace I2C_DRIVER

#endif // I2C_DRIVER_H

// ============================================================================
// IMPLEMENTATION
// ============================================================================

namespace I2C_DRIVER {

    // Global FSM pointers (For ISR routing)
    I2C_FSM* g_i2c1_fsm = nullptr;
    I2C_FSM* g_i2c2_fsm = nullptr;
    I2C_FSM* g_i2c3_fsm = nullptr;

    // INITIALIZATION
    // ============================================================================

    void i2c_init(I2C_TypeDef* I2C, uint16_t speed_kHz, uint32_t pclk1_hz) {
        // 1️⃣ Enable GPIO Clock
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; // For I2C1/I2C2 on GPIOB

        // 2️⃣ Configure GPIO Pins (Open-Drain, Alternate Function 4)
        if (I2C == I2C1) {
            RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
            // PB6 (SCL), PB7 (SDA)
            GPIOB->MODER &= ~(GPIO_MODER_MODE6_Msk | GPIO_MODER_MODE7_Msk);
            GPIOB->MODER |= (0b10 << GPIO_MODER_MODE6_Pos) | (0b10 << GPIO_MODER_MODE7_Pos);
            GPIOB->OTYPER |= GPIO_OTYPER_OT6 | GPIO_OTYPER_OT7;
            GPIOB->OSPEEDR |= (0b11 << GPIO_OSPEEDR_OSPEED6_Pos) | (0b11 << GPIO_OSPEEDR_OSPEED7_Pos);
            GPIOB->AFR[0] &= ~(GPIO_AFRL_AFSEL6_Msk | GPIO_AFRL_AFSEL7_Msk);
            GPIOB->AFR[0] |= (0b100 << GPIO_AFRL_AFSEL6_Pos) | (0b100 << GPIO_AFRL_AFSEL7_Pos);
        } else if (I2C == I2C2) {
            RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;
            // PB10 (SCL), PB11 (SDA)
            GPIOB->MODER &= ~(GPIO_MODER_MODE10_Msk | GPIO_MODER_MODE11_Msk);
            GPIOB->MODER |= (0b10 << GPIO_MODER_MODE10_Pos) | (0b10 << GPIO_MODER_MODE11_Pos);
            GPIOB->OTYPER |= GPIO_OTYPER_OT10 | GPIO_OTYPER_OT11;
            GPIOB->OSPEEDR |= (0b11 << GPIO_OSPEEDR_OSPEED10_Pos) | (0b11 << GPIO_OSPEEDR_OSPEED11_Pos);
            GPIOB->AFR[1] &= ~(GPIO_AFRH_AFSEL10_Msk | GPIO_AFRH_AFSEL11_Msk);
            GPIOB->AFR[1] |= (0b100 << GPIO_AFRH_AFSEL10_Pos) | (0b100 << GPIO_AFRH_AFSEL11_Pos);
        } else if (I2C == I2C3) {
            RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
            RCC->APB1ENR |= RCC_APB1ENR_I2C3EN;
            // PA8 (SCL), PB5 (SDA) - Note: Mixed ports
            GPIOA->MODER &= ~GPIO_MODER_MODE8_Msk;
            GPIOA->MODER |= (0b10 << GPIO_MODER_MODE8_Pos);
            GPIOA->OTYPER |= GPIO_OTYPER_OT8;
            GPIOA->OSPEEDR |= (0b11 << GPIO_OSPEEDR_OSPEED8_Pos);
            GPIOA->AFR[1] &= ~GPIO_AFRH_AFSEL8_Msk;
            GPIOA->AFR[1] |= (4U << GPIO_AFRH_AFSEL8_Pos);

            GPIOB->MODER &= ~GPIO_MODER_MODE5_Msk;
            GPIOB->MODER |= (0b10 << GPIO_MODER_MODE5_Pos);
            GPIOB->OTYPER |= GPIO_OTYPER_OT5;
            GPIOB->OSPEEDR |= (0b11 << GPIO_OSPEEDR_OSPEED5_Pos);
            GPIOB->AFR[0] &= ~GPIO_AFRL_AFSEL5_Msk;
            GPIOB->AFR[0] |= (9U << GPIO_AFRL_AFSEL5_Pos);
        }

        __DSB();

        // 3️⃣ Peripheral Reset
        I2C->CR1 |= I2C_CR1_SWRST;
        I2C->CR1 &= ~I2C_CR1_SWRST;

        // 4️⃣ Configure Clock
        uint32_t pclk1_mhz = pclk1_hz / 1000000;
        I2C->CR2 = (pclk1_mhz & I2C_CR2_FREQ_Msk);

        // 5️⃣ Set Speed & Rise Time
        if (speed_kHz == 100) {
            I2C->CCR = pclk1_hz / 200000; // Standard mode
            I2C->TRISE = pclk1_mhz + 1;
        } else if (speed_kHz == 400) {
            I2C->CCR = I2C_CCR_FS | (pclk1_hz / 1200000); // Fast mode, 2:1 duty
            I2C->TRISE = ((pclk1_mhz * 300) / 1000) + 1;
        }

        // 6️⃣ Enable Interrupts
        I2C->CR2 |= (I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN);

        // 7️⃣ Configure NVIC
        if (I2C == I2C1) {
            NVIC_SetPriority(I2C1_EV_IRQn, 2);
            NVIC_EnableIRQ(I2C1_EV_IRQn);
            NVIC_SetPriority(I2C1_ER_IRQn, 3);
            NVIC_EnableIRQ(I2C1_ER_IRQn);
        } else if (I2C == I2C2) {
            NVIC_SetPriority(I2C2_EV_IRQn, 2);
            NVIC_EnableIRQ(I2C2_EV_IRQn);
            NVIC_SetPriority(I2C2_ER_IRQn, 3);
            NVIC_EnableIRQ(I2C2_ER_IRQn);
        } else if (I2C == I2C3) {
            NVIC_SetPriority(I2C3_EV_IRQn, 2);
            NVIC_EnableIRQ(I2C3_EV_IRQn);
            NVIC_SetPriority(I2C3_ER_IRQn, 3);
            NVIC_EnableIRQ(I2C3_ER_IRQn);
        }

        // 8️⃣ Enable Peripheral
        I2C->CR1 |= I2C_CR1_PE;
    }

    // TRANSACTION START
    // ============================================================================

    I2C_Status i2c_master_write_async(I2C_TypeDef* I2C, I2C_FSM* fsm,
                                      uint8_t addr, uint8_t* data, uint16_t size,
                                      uint32_t timeout, I2C_CompletionCallback callback) {
        // ✅ Validate Parameters
        if (!fsm || !data || size == 0 || size > I2C_MAX_BUFFER_SIZE) {
            return I2C_ERR_PARAM;
        }
        if (addr < I2C_ADDR_MIN || addr > I2C_ADDR_MAX) {
            return I2C_ERR_ADDR;
        }
        if (fsm->current_state != I2C_STATE_IDLE) {
            return I2C_ERR_BUSY; // Previous transaction still in progress
        }

        // Handle Errata 2.9.3: Bus Stuck in BUSY state
        if (i2c_is_busy(I2C) && fsm->current_state == I2C_STATE_IDLE) {
            i2c_software_reset(I2C);
            // Re-init clock timing if necessary
        }

        // Setup FSM
        fsm->dev_addr = addr;
        fsm->tx_buffer = data;
        fsm->tx_bytes = size;
        fsm->tx_index = 0;
        fsm->rx_bytes = 0; // Write-only
        fsm->timeout_ms = timeout ? timeout : I2C_DEFAULT_TIMEOUT_MS;
        fsm->start_tick = i2c_get_tick_ms();
        fsm->callback = callback;
        fsm->callback_data = nullptr;
        fsm->retry_count = 0;
        fsm->last_error = I2C_OK;

        // Start Transaction
        fsm->current_state = I2C_STATE_WRITE;
        i2c_start(I2C);

        return I2C_OK;
    }

    I2C_Status i2c_master_read_async(I2C_TypeDef* I2C, I2C_FSM* fsm,
                                     uint8_t addr, uint8_t* reg, uint16_t reg_size,
                                     uint8_t* rx_buf, uint16_t rx_size,
                                     uint32_t timeout, I2C_CompletionCallback callback) {
        // Validate Parameters
        if (!fsm || !reg || reg_size == 0 || !rx_buf || rx_size == 0) {
            return I2C_ERR_PARAM;
        }
        if (reg_size > I2C_MAX_BUFFER_SIZE || rx_size > I2C_MAX_BUFFER_SIZE) {
            return I2C_ERR_PARAM;
        }
        if (addr < I2C_ADDR_MIN || addr > I2C_ADDR_MAX) {
            return I2C_ERR_ADDR;
        }
        if (fsm->current_state != I2C_STATE_IDLE && fsm->current_state != I2C_STATE_ERROR) {
            return I2C_ERR_BUSY;
        }

        // Handle Errata 2.9.3
        if (i2c_is_busy(I2C) && fsm->current_state == I2C_STATE_IDLE) {
            i2c_software_reset(I2C);
        }

        // Setup FSM
        fsm->dev_addr = addr;
        fsm->tx_buffer = reg; // Register address to write first
        fsm->tx_bytes = reg_size;
        fsm->tx_index = 0;
        fsm->rx_buffer = rx_buf; // Data to read after restart
        fsm->rx_bytes = rx_size;
        fsm->rx_index = 0;
        fsm->timeout_ms = timeout ? timeout : I2C_DEFAULT_TIMEOUT_MS;
        fsm->start_tick = i2c_get_tick_ms();
        fsm->callback = callback;
        fsm->callback_data = nullptr;
        fsm->retry_count = 0;
        fsm->last_error = I2C_OK;

        // Ensure POS bit is cleared from previous 2-byte reads
        I2C->CR1 &= ~I2C_CR1_POS;
        i2c_ack(I2C);

        // Start Transaction
        fsm->current_state = I2C_STATE_WRITE;
        i2c_start(I2C);

        return I2C_OK;
    }

    // FSM STATE MACHINE
    // ============================================================================

    void i2c_fsm_process(I2C_FSM* fsm, I2C_TypeDef* I2C) {
        if (!fsm) return;

        // TIMEOUT CHECK (CRITICAL!) - Do this first in all states
        uint32_t elapsed = i2c_get_tick_ms() - fsm->start_tick;
        if (elapsed > fsm->timeout_ms && fsm->current_state != I2C_STATE_IDLE) {
            fsm->current_state = I2C_STATE_ERROR;
            fsm->last_error = I2C_ERR_TIMEOUT;
            i2c_stop(I2C);
            // Trigger callback if registered
            if (fsm->callback) {
                fsm->callback(I2C_ERR_TIMEOUT, fsm->callback_data);
            }
            return;
        }

        uint32_t sr1 = I2C->SR1;

        switch (fsm->current_state) {

        // WRITE PHASE: Send slave address in write mode
        case I2C_STATE_WRITE:
            if (sr1 & I2C_SR1_SB) {
                // START condition sent, now send slave address + write bit
                i2c_send_addr(I2C, fsm->dev_addr, true); // true = write
                // Stay in WRITE state until ADDR flag
            }
            if (sr1 & I2C_SR1_ADDR) {
                // Address matched, clear ADDR flag
                i2c_clear_addr(I2C);
                fsm->tx_index = 0;

                // Send first data byte (register address or data)
                if (fsm->tx_bytes > 0) {
                    i2c_send_byte(I2C, fsm->tx_buffer[fsm->tx_index++]);
                }
            }
            if ((sr1 & I2C_SR1_TXE) && fsm->tx_index < fsm->tx_bytes) {
                // TX buffer empty, send next byte
                i2c_send_byte(I2C, fsm->tx_buffer[fsm->tx_index++]);
            }
            if ((sr1 & I2C_SR1_BTF) && fsm->tx_index >= fsm->tx_bytes) {
                // All TX bytes sent, decide next state
                if (fsm->rx_bytes > 0) {
                    // Need to read: issue repeated START
                    fsm->current_state = I2C_STATE_RESTART;
                    I2C->CR2 &= ~I2C_CR2_ITBUFEN; // Disable buffer interrupt
                    i2c_start(I2C);
                } else {
                    // Write-only: send STOP
                    fsm->current_state = I2C_STATE_STOP;
                    i2c_stop(I2C);
                }
            }
            break;

        // RESTART: Send repeated START for read
        case I2C_STATE_RESTART:
            if (sr1 & I2C_SR1_SB) {
                // START sent, now send address in read mode
                I2C->CR2 |= I2C_CR2_ITBUFEN;              // Re-enable buffer interrupt
                i2c_send_addr(I2C, fsm->dev_addr, false); // false = read
                fsm->current_state = I2C_STATE_READ;
            }
            break;

        // READ PHASE: Receive data from slave
        case I2C_STATE_READ:
            if (sr1 & I2C_SR1_ADDR) {
                // Address matched in read mode
                // Special handling for 1-byte and 2-byte reads (I2C quirks)
                if (fsm->rx_bytes == 1) {
                    i2c_nack(I2C); // Don't ACK single byte
                    i2c_clear_addr(I2C);
                    i2c_stop(I2C); // Send STOP immediately
                } else if (fsm->rx_bytes == 2) {
                    // For 2-byte: Set POS, disable ACK, then clear ADDR
                    I2C->CR1 |= I2C_CR1_POS;
                    i2c_nack(I2C);
                    i2c_clear_addr(I2C);
                    i2c_stop(I2C);
                } else {
                    // For N > 2 bytes: Enable ACK
                    i2c_ack(I2C);
                    i2c_clear_addr(I2C);
                }
                fsm->rx_index = 0;
            }

            if (sr1 & I2C_SR1_RXNE) {
                // Data received

                // Special case: Last 2 bytes of N-byte read
                if (fsm->rx_bytes > 2 && fsm->rx_index == (fsm->rx_bytes - 2)) {
                    if (sr1 & I2C_SR1_BTF) {
                        i2c_nack(I2C);
                        fsm->rx_buffer[fsm->rx_index++] = i2c_read_byte(I2C);
                        i2c_stop(I2C);
                    }
                }
                // Special case: 2-byte read (both bytes ready)
                else if (fsm->rx_bytes == 2 && fsm->rx_index == 0) {
                    if (sr1 & I2C_SR1_BTF) {
                        fsm->rx_buffer[fsm->rx_index++] = i2c_read_byte(I2C);
                    }
                }
                // Standard read: single byte
                else {
                    fsm->rx_buffer[fsm->rx_index++] = i2c_read_byte(I2C);
                    if (fsm->rx_index >= fsm->rx_bytes) {
                        fsm->current_state = I2C_STATE_STOP;
                    }
                }
            }
            break;

        // STOP: Cleanup
        case I2C_STATE_STOP:
            fsm->current_state = I2C_STATE_IDLE;
            I2C->CR1 &= ~I2C_CR1_POS;    // Clear POS bit
            I2C->CR2 |= I2C_CR2_ITBUFEN; // Re-enable for next transaction

            // Trigger callback with success
            if (fsm->callback) {
                fsm->callback(I2C_OK, fsm->callback_data);
            }
            break;

        // ERROR: Attempt recovery
        case I2C_STATE_ERROR:
            i2c_stop(I2C);
            fsm->current_state = I2C_STATE_IDLE;

            // Trigger callback with error
            if (fsm->callback && fsm->last_error != I2C_OK) {
                fsm->callback(fsm->last_error, fsm->callback_data);
            }
            break;

        case I2C_STATE_IDLE:
        default:
            break;
        }
    }

    // ERROR & RECOVERY
    // ============================================================================

    void i2c_bus_recovery(I2C_TypeDef* I2C, GPIO_TypeDef* scl_port, uint16_t scl_pin,
                          GPIO_TypeDef* sda_port, uint16_t sda_pin) {
        //  Reference: https://www.zyma.me/post/stm32f4-i2c-busy-after-software-reset/

        //  Reconfigure I2C pins as GPIO output (open-drain)
        uint32_t scl_moder_pos = scl_pin * 2;
        uint32_t sda_moder_pos = sda_pin * 2;

        scl_port->MODER &= ~(3U << scl_moder_pos);
        scl_port->MODER |= (1U << scl_moder_pos); // Output mode
        scl_port->OTYPER |= (1U << scl_pin);      // Open-drain

        sda_port->MODER &= ~(3U << sda_moder_pos);
        sda_port->MODER |= (1U << sda_moder_pos); // Output mode
        sda_port->OTYPER |= (1U << sda_pin);      // Open-drain

        // 2️⃣ Hold SDA high (so slave sees NACK at end)
        sda_port->ODR |= (1U << sda_pin);

        // 3️⃣ Generate clock pulses on SCL
        for (int i = 0; i < I2C_RECOVERY_PULSES; ++i) {
            scl_port->ODR |= (1U << scl_pin); // SCL high
            for (volatile int j = 0; j < 1000; ++j) __NOP();
            scl_port->ODR &= ~(1U << scl_pin); // SCL low
            for (volatile int j = 0; j < 1000; ++j) __NOP();
        }

        // 4️⃣ Final SCL high
        scl_port->ODR |= (1U << scl_pin);
        for (volatile int j = 0; j < 1000; ++j) __NOP();

        // 5️⃣ Reconfigure pins back to I2C alternate function
        // (You'd call i2c_init() again or do selective GPIO reconfiguration)
    }

    void i2c_software_reset(I2C_TypeDef* I2C) {
        // Clear PE bit to reset internal state machines
        I2C->CR1 &= ~I2C_CR1_PE;
        __DSB();
        I2C->CR1 |= I2C_CR1_PE;
        __DSB();
    }

    const char* i2c_status_str(I2C_Status status) {
        switch (status) {
        case I2C_OK: return "✅ I2C_OK";
        case I2C_ERR_BUSY: return "🚫 I2C_ERR_BUSY";
        case I2C_ERR_PARAM: return "⚠️  I2C_ERR_PARAM";
        case I2C_ERR_ADDR: return "⚠️  I2C_ERR_ADDR";
        case I2C_ERR_TIMEOUT: return "⏰ I2C_ERR_TIMEOUT";
        case I2C_ERR_NACK: return "❌ I2C_ERR_NACK";
        case I2C_ERR_BUS: return "⛔ I2C_ERR_BUS";
        default: return "❓ I2C_ERR_UNKNOWN";
        }
    }

    // ISR HANDLERS
    // ============================================================================

    void i2c_handle_event_irq(I2C_FSM* fsm, I2C_TypeDef* I2C) {
        if (fsm) {
            i2c_fsm_process(fsm, I2C);
        }
    }

    void i2c_handle_error_irq(I2C_FSM* fsm, I2C_TypeDef* I2C) {
        uint32_t sr1 = I2C->SR1;

        // Handle NACK (Slave doesn't respond)
        if (sr1 & I2C_SR1_AF) {
            I2C->SR1 &= ~I2C_SR1_AF;
            if (fsm) fsm->last_error = I2C_ERR_NACK;
        }

        // Errata 2.4.6: Spurious Bus Error in Master Mode
        // Simply clear the flag without further action
        if (sr1 & I2C_SR1_BERR) {
            I2C->SR1 &= ~I2C_SR1_BERR;
            if (fsm) fsm->last_error = I2C_ERR_BUS;
        }

        // Arbitration Loss
        if (sr1 & I2C_SR1_ARLO) {
            I2C->SR1 &= ~I2C_SR1_ARLO;
            if (fsm) fsm->last_error = I2C_ERR_BUS;
        }

        // Move to error state
        if (fsm && fsm->current_state != I2C_STATE_IDLE) {
            fsm->current_state = I2C_STATE_ERROR;
        }

        // Recover stuck BUSY if needed
        if ((I2C->SR2 & I2C_SR2_BUSY) && fsm && fsm->current_state == I2C_STATE_IDLE) {
            i2c_software_reset(I2C);
        }

        // Process error state
        if (fsm) {
            i2c_fsm_process(fsm, I2C);
        }
    }

} // namespace I2C_DRIVER

// ============================================================================
// ISR INSTANTIATIONS (in your main project)
// ============================================================================
/*

extern "C" {

void I2C1_EV_IRQHandler(void) {
    I2C_DRIVER::i2c_handle_event_irq(I2C_DRIVER::g_i2c1_fsm, I2C1);
}

void I2C1_ER_IRQHandler(void) {
    I2C_DRIVER::i2c_handle_error_irq(I2C_DRIVER::g_i2c1_fsm, I2C1);
}

void I2C2_EV_IRQHandler(void) {
    I2C_DRIVER::i2c_handle_event_irq(I2C_DRIVER::g_i2c2_fsm, I2C2);
}

void I2C2_ER_IRQHandler(void) {
    I2C_DRIVER::i2c_handle_error_irq(I2C_DRIVER::g_i2c2_fsm, I2C2);
}

// Get system tick (implement based on your RTOS)
namespace I2C_DRIVER {
    uint32_t i2c_get_tick_ms(void) {
        return HAL_GetTick();  // Or your RTOS equivalent
    }
}

}
*/
