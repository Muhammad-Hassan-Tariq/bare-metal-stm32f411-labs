# 06 - I2C Based Communication

## Overview
- This project demonstrates a **USART based data Transmission** on the STM32F411 MCU alongwith Arduino UNO.

## Hardware Setup  
- 1x Logic Analyzer
- picocom Software
- 2x 4.7K Resistors to Pull Up Voltage
- Wire Arduino  |  STM32F411
       GND      |  GND
       SDA      |  PB6
       SDC      |  PB7

## Working  
- Arduino Initializes in picocom 9600 Baud with `I2C Target Ready at 0x3C` and spits out Byte that it recieves.
- Using `sudo picocom -b 115200 /dev/ttyUSB0` Transmission can be seen in terminal.Terminal.
- To see realtime clock and data pulses Logic Analyzer might be required.
