# 11 - Non Blocking Interrupt Driven USART DRIVER

## Overview
- This project demonstrates a **Non Blocking USART DRIVER** on the STM32F411 MCU.

## Hardware Setup  
- 1x USB-TTL Adaptor
- picocom Software
- Wire USB-TTL  |  STM32F411
       GND      |  GND
       RX       |  PA10
       TX       |  PA9

## Working  
- DWT counter used for microsecond Precision
- Using `sudo picocom -b 115200 /dev/ttyUSB0` Transmission can be seen in terminal.
