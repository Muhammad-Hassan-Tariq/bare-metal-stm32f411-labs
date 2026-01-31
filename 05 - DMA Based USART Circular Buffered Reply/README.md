# 04 - UART based Transmission

## Overview
- This project demonstrates a **USART based data Transmission** on the STM32F411 MCU.

## Hardware Setup  
- 1x USB-TTL Adaptor
- picocom Software
- Wire USB-TTL  |  STM32F411
       GND      |  GND
       RX       |  PA2

## Working  
- SysTick interrupts every 1ms to change flag status.
- Using `sudo picocom -b 115200 /dev/ttyUSB0` Transmission can be seen in terminal.
