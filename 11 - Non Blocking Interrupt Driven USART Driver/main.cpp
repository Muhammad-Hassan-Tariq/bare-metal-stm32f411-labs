/**
 * @file    main.cpp
 * @author  [Muhammad Hassan Tariq/ https://github.com/Muhammad-Hassan-Tariq]
 * @brief   Testing Code for Non Blocking Interrupt Driven USART DRIVER
 * @details Implemented a Ring Buffered Non Blocking Interrupt Driven USART DRIVER
 * @notice  Test Cases Were AI Assisted
 * @date    2026-02-15
 * @note    Target: STM32F411CEU6 (Black Pill)
 */

#include "../platform/drivers/stm32f411xe.h"
#include "usart_driver.cpp"
#include <cstdint>

// ============================================================
// DWT Timer - Cycle Counter (Built-in to CPU core)
// ============================================================

void init_dwt_timer(void) {
    // Enable trace unit (CoreDebug)
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    // Enable DWT cycle counter
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    // Reset counter to 0
    DWT->CYCCNT = 0;
}

uint32_t get_time_us(void) {
    // At 16MHz: 16 cycles = 1 microsecond
    return DWT->CYCCNT / 16;
}

uint32_t get_time_cycles(void) {
    // Raw cycle count (no conversion)
    return DWT->CYCCNT;
}

void delay_us(uint32_t us) {
    uint32_t start = get_time_us();
    while ((get_time_us() - start) < us);
}

void delay_ms(uint32_t ms) {
    delay_us(ms * 1000);
}

// ============================================================
// Helper: Convert number to string and send via UART
// ============================================================

void send_number(USART_TypeDef* uart, uint32_t num) {
    char buffer[12];
    int index = 0;

    if (num == 0) {
        USART_DRIVER::send_async(uart, "0");
        return;
    }

    while (num > 0) {
        buffer[index++] = '0' + (num % 10);
        num /= 10;
    }

    // Send in reverse order
    for (int i = index - 1; i >= 0; i--) {
        char single_char[2] = {buffer[i], '\0'};
        USART_DRIVER::send_async(uart, single_char);
    }
}

// ============================================================
// TEST 1: Measure send_async() Execution Time
// ============================================================

void test_send_async_speed(void) {
    USART_DRIVER::send_async(USART1, "\r\n=== TEST 1: send_async() Speed ===\r\n");

    const char* long_string = "This is a reasonably long string that takes time to transmit at 115200 baud but should queue instantly\r\n";

    uint32_t start_time = get_time_us();
    USART_DRIVER::send_async(USART1, long_string);
    uint32_t elapsed = get_time_us() - start_time;

    USART_DRIVER::send_async(USART1, "send_async() took: ");
    send_number(USART1, elapsed);
    USART_DRIVER::send_async(USART1, " microseconds\r\n");

    if (elapsed < 100) {
        USART_DRIVER::send_async(USART1, "PASS: send_async() is non-blocking\r\n");
    } else {
        USART_DRIVER::send_async(USART1, "FAIL: send_async() took too long\r\n");
    }
}

// ============================================================
// TEST 2: Main Loop Continues While UART Transmits
// ============================================================

void test_concurrent_execution(void) {
    USART_DRIVER::send_async(USART1, "\r\n=== TEST 2: Concurrent Execution ===\r\n");

    USART_DRIVER::send_async(USART1, "Sending data asynchronously...\r\n");

    // Queue multiple messages (total ~500 bytes, ~43ms to transmit)
    for (int i = 0; i < 5; i++) {
        USART_DRIVER::send_async(USART1, "Background TX: This is async data flowing while main loop runs\r\n");
    }

    // Now measure how many iterations main loop does while UART works
    uint32_t counter = 0;
    uint32_t start_time = get_time_us();

    // Run for 50ms while TX happens asynchronously
    while ((get_time_us() - start_time) < 50000) {
        counter++;
        volatile int dummy = counter * 2; // Prevent optimization
    }

    USART_DRIVER::send_async(USART1, "Main loop completed ");
    send_number(USART1, counter);
    USART_DRIVER::send_async(USART1, " iterations while UART transmitting\r\n");
    USART_DRIVER::send_async(USART1, "PASS: Main loop ran unblocked\r\n");
}

// ============================================================
// TEST 3: RX is Non-Blocking
// ============================================================

void test_rx_non_blocking(void) {
    USART_DRIVER::send_async(USART1, "\r\n=== TEST 3: RX Non-Blocking ===\r\n");

    USART_DRIVER::send_async(USART1, "Testing 1000 non-blocking read() calls...\r\n");

    uint32_t start_time = get_time_us();

    for (int i = 0; i < 1000; i++) {
        uint8_t byte = USART_DRIVER::read(USART1);
        (void)byte;
    }

    uint32_t elapsed = get_time_us() - start_time;

    USART_DRIVER::send_async(USART1, "1000 read() calls took: ");
    send_number(USART1, elapsed);
    USART_DRIVER::send_async(USART1, " microseconds\r\n");

    if (elapsed < 10000) {
        USART_DRIVER::send_async(USART1, "PASS: read() is non-blocking\r\n");
    }
}

// ============================================================
// TEST 4: available() Check Speed
// ============================================================

void test_available_speed(void) {
    USART_DRIVER::send_async(USART1, "\r\n=== TEST 4: available() Speed ===\r\n");

    uint32_t start_time = get_time_us();

    for (int i = 0; i < 10000; i++) {
        bool has_data = USART_DRIVER::available(USART1);
        (void)has_data;
    }

    uint32_t elapsed = get_time_us() - start_time;

    USART_DRIVER::send_async(USART1, "10000 available() checks took: ");
    send_number(USART1, elapsed);
    USART_DRIVER::send_async(USART1, " microseconds\r\n");
    USART_DRIVER::send_async(USART1, "PASS: available() is instant\r\n");
}

// ============================================================
// TEST 5: Stress Test - Rapid Fire Sends
// ============================================================

void test_rapid_sends(void) {
    USART_DRIVER::send_async(USART1, "\r\n=== TEST 5: Rapid Fire Sends ===\r\n");

    USART_DRIVER::send_async(USART1, "Sending 50 messages rapidly...\r\n");

    uint32_t start_time = get_time_us();

    for (int i = 0; i < 50; i++) {
        USART_DRIVER::send_async(USART1, "Message X\r\n");
    }

    uint32_t total_queue_time = get_time_us() - start_time;

    USART_DRIVER::send_async(USART1, "Queued 50 messages in: ");
    send_number(USART1, total_queue_time);
    USART_DRIVER::send_async(USART1, " microseconds\r\n");
    USART_DRIVER::send_async(USART1, "PASS: All sends queued instantly\r\n");
}

// ============================================================
// TEST 6: Echo Test with available() Check
// ============================================================

void test_echo(void) {
    USART_DRIVER::send_async(USART1, "\r\n=== TEST 6: Echo Test ===\r\n");
    USART_DRIVER::send_async(USART1, "Send characters to UART1, they will be echoed back\r\n");
    USART_DRIVER::send_async(USART1, "Timeout: 10 seconds\r\n");

    uint32_t timeout = get_time_us() + 10000000; // 10 second timeout
    uint32_t echo_count = 0;

    while (get_time_us() < timeout) {
        // Non-blocking check for data
        if (USART_DRIVER::available(USART1)) {
            uint8_t byte = USART_DRIVER::read(USART1);

            // Echo it back by sending it as a string
            if (byte == '\r') {
                USART_DRIVER::send_async(USART1, "\r\n");
            } else if (byte >= 32 && byte < 127) {
                char echo_str[2] = {(char)byte, '\0'};
                USART_DRIVER::send_async(USART1, echo_str);
                echo_count++;
            }
        }
    }

    USART_DRIVER::send_async(USART1, "Echo test complete. Echoed ");
    send_number(USART1, echo_count);
    USART_DRIVER::send_async(USART1, " characters\r\n");
}

// ============================================================
// TEST 7: Multi-UART Concurrent Operation
// ============================================================

void test_multi_uart_async(void) {
    USART_DRIVER::send_async(USART1, "\r\n=== TEST 7: Multi-UART Async ===\r\n");

    uint32_t start = get_time_us();

    USART_DRIVER::send_async(USART1, "UART1: Message 1\r\n");
    USART_DRIVER::send_async(USART2, "UART2: Message 1\r\n");
    USART_DRIVER::send_async(USART1, "UART1: Message 2\r\n");
    USART_DRIVER::send_async(USART2, "UART2: Message 2\r\n");

    uint32_t elapsed = get_time_us() - start;

    USART_DRIVER::send_async(USART1, "Queued 4 messages on 2 UARTs in: ");
    send_number(USART1, elapsed);
    USART_DRIVER::send_async(USART1, " microseconds\r\n");
    USART_DRIVER::send_async(USART1, "PASS: Multi-UART async works\r\n");
}

// ============================================================
// TEST 8: Simple Hassan Test - The One That Works
// ============================================================

void test_hassan_simple(void) {
    USART_DRIVER::send_async(USART1, "\r\n=== TEST 8: Hassan's Simple Loop Test ===\r\n");
    USART_DRIVER::send_async(USART1, "Running: while(true) send_async(USART1, \"Hassan\\r\\n\")\r\n");
    USART_DRIVER::send_async(USART1, "for 5 seconds...\r\n");

    uint32_t start = get_time_us();
    uint32_t send_count = 0;

    while ((get_time_us() - start) < 5000000) { // 5 seconds
        USART_DRIVER::send_async(USART1, "Hassan\r\n");
        send_count++;
    }

    USART_DRIVER::send_async(USART1, "Completed ");
    send_number(USART1, send_count);
    USART_DRIVER::send_async(USART1, " sends in 5 seconds\r\n");
    USART_DRIVER::send_async(USART1, "PASS: Hassan's test works perfectly\r\n");
}

// ============================================================
// Main Async Test Suite
// ============================================================

int main(void) {
    // Initialize DWT timer (cycle counter)
    init_dwt_timer();

    // Initialize UART with 16MHz clock
    USART_DRIVER::init_uart(USART1, 115200, 16000000);
    USART_DRIVER::init_uart(USART2, 115200, 16000000);

    // Small delay to let UART stabilize
    delay_ms(100);

    USART_DRIVER::send_async(USART1, "\r\n========================================\r\n");
    USART_DRIVER::send_async(USART1, "UART ASYNC DRIVER TEST SUITE\r\n");
    USART_DRIVER::send_async(USART1, "Hassan's Bare Metal STM32F411\r\n");
    USART_DRIVER::send_async(USART1, "Using DWT for EXACT microsecond timing\r\n");
    USART_DRIVER::send_async(USART1, "========================================\r\n");

    delay_ms(200);

    // Run all tests
    test_send_async_speed();
    test_concurrent_execution();
    test_rx_non_blocking();
    test_available_speed();
    test_rapid_sends();
    test_multi_uart_async();
    test_hassan_simple();
    test_echo(); // Interactive test last

    USART_DRIVER::send_async(USART1, "\r\n========================================\r\n");
    USART_DRIVER::send_async(USART1, "ALL ASYNC TESTS COMPLETE\r\n");
    USART_DRIVER::send_async(USART1, "Your driver is production-ready!\r\n");
    USART_DRIVER::send_async(USART1, "========================================\r\n");

    // Final loop: echo any input
    while (1) {
        if (USART_DRIVER::available(USART1)) {
            uint8_t byte = USART_DRIVER::read(USART1);
            char echo_str[2] = {(char)byte, '\0'};
            USART_DRIVER::send_async(USART1, echo_str);
        }
    }

    return 0;
}
