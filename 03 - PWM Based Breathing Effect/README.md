# 01 - Interrupt Based Binary Counter

## Overview
- This project demonstrates a **PWM based Breathing Effect** on the STM32F411 MCU.

## Hardware Setup  
- 1x LED
- 1x 220 ohm Resistor
- Wire LED to PIN A0

## Working  
- SysTick interrupts every 1ms adding or removing from a step.
- It takes Up (0→1000): 1000 / 5 = 200 interrupts = 200ms & vice versa.
- Overall LED shows a full breathing effect per 400 ms.
