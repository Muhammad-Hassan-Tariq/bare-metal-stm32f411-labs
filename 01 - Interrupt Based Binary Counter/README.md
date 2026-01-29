# 01 - Interrupt Based Binary Counter

## Overview
- This project demonstrates a **binary counter using GPIO interrupts** on the STM32F411 MCU.

## Hardware Setup  
- 5x LEDs [3x Green & 2x Red] are used along with 5x 220 ohm resistors.  
- A tactile push button is used for Input.
- LEDs are mapped from Pin A0 to A4 & push button is mapped to PB0.

## Working  
- LEDs represent 0 / 1 in their state.
- EXTI is configured on Pin B0 at Falling trigger.
- The counter increments when a button is pressed, changing LED states connected to the GPIO pins.  
- CPU sits idle and when falling edge interrupt occurs it changes states of LEDs.  
