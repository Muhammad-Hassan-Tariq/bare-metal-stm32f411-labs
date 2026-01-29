# 02 - DMA Based Copying Array  

## Overview
- This project demonstrates a **DMA based array copy** on the STM32F411 MCU.

## Hardware Setup  
- Builtin LED on Pin C13 works as a flag.  

## Working  
- A flag named dma_status is initialized to false
- LED is off initially and a delay is added to see what's going on.  
- After inital delay DMA is initialized to copy array data from source to destination.  
- Once DMA completes copying data it triggers an interrupt on Stream0 which changes flag dma_status.  
- The moment condition of dma_status becomes true, the while loop starts toggling led for rest of it's life.  
