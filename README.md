![STM32](https://img.shields.io/badge/STM32-F411-blue)
![ARM](https://img.shields.io/badge/ARM-Cortex--M4-orange)
![Language](https://img.shields.io/badge/Language-C%2B%2B-green)
![Build](https://img.shields.io/badge/Build-Make-red)
![IMG_20260114_140556_768 (1)_1](https://github.com/user-attachments/assets/8243e93a-22cf-4189-a1d8-aa3dd9c43f7c)

## 📌 STM32 Bare-Metal Learning Labs

This repository contains a collection of **bare-metal STM32F411 experiments**
implemented using **CMSIS** (no HAL or LL drivers). It's my progression of learning Bare
Metal MCU programming

Each subdirectory is a self-contained lab focused on **one peripheral or system
concept and it's practical use**

Target MCU: **STM32F411 (Cortex-M4) aka The Black Pill**

---

## 🎯 Goals of This Repository

- Understand STM32 at **register level**
- Learn how peripherals interact (DMA ↔ Timer ↔ GPIO ↔ NVIC)
- Avoid dependency on vendor abstraction layers
- How to use best practices in **c++** for maximum efficiency

This is not production firmware — it is a **silicon exploration lab**.

---

## 🗂 Projects [Other will be added soon this repo is not complete yet]
- [01 - Interrupt Based Binary Counter](https://github.com/Muhammad-Hassan-Tariq/Bare-Metal-stm32f411-projects/tree/main/01%20-%20Interrupt%20Based%20Binary%20Counter)
- [02 - DMA Based Copying Array](https://github.com/Muhammad-Hassan-Tariq/Bare-Metal-stm32f411-projects/tree/main/02%20-%20DMA%20Based%20Copying%20Array)
- [03 - 03 - PWM Based Breathing Effect](https://github.com/Muhammad-Hassan-Tariq/Bare-Metal-stm32f411-projects/tree/main/03%20-%20PWM%20Based%20Breathing%20Effect)

---

### 📁 Project Directories

Each project directory consists of:

- `main.cpp`  
  Main project source code

- `schematic.svg`  [It may or may not be present based on project itself]  
  Schematic diagram to show wiring & overall working 
  
- `Makefile`  
  A make file responsible for compiling, linking and flashing with one command
  
- `README.md`  
  Explanation of:
  - what the project demonstrates
  - peripheral configuration summary
  - register-level flow

Projects are designed to be **read independently**.

---

## ⚙️ Toolchain Used

All projects use the same base toolchain:

- Compiler: `arm-none-eabi-gcc`
- Assembler: `arm-none-eabi-as`
- Linker script: `/platform/STM32F411CEU6_FLASH.ld`
- Startup code: `/platform/startup_stm32f411xe.s`

Flashing via:

- `dfu-util`
- `OpenOCD`  

Build system: simple `Makefile` per project

---

## ▶️ Getting Started

- **Install Tools**  
  *-> For **Debian** based OS*
  - `sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi dfu-util git make openocd`
  
  *-> For **Red Hat** based OS*
  - `sudo dnf install arm-none-eabi-gcc-cs arm-none-eabi-binutils-cs arm-none-eabi-newlib dfu-util git make openocd`
  
  *-> For **Arch** based OS*
  - `sudo pacman -S arm-none-eabi-gcc arm-none-eabi-binutils arm-none-eabi-newlib dfu-util git make openocd`
  
- **Verify Installation**
  - `arm-none-eabi-g++ --version`
  
- **Pull this repository to local system**
  - `git pull https://github.com/Muhammad-Hassan-Tariq/Bare-Metal-stm32f411-projects.git`
    

- **Build & Flash**
    - Change directory to any project listed
    - `sudo make dfu`
    - `sudo make openocd_flash`  

---

## 💻 My Development Environment

- **OS:** Arch Linux
- **Editor:** Neovim + clangd LSP
- **Build:** GNU Make
- **Debug:** GDB + ST-Link V2
- **Language:** C++ (with C-style register access)

**Why C++ for embedded?**
- Type safety without performance overhead
- constexpr for compile-time optimization
- Templates for zero-cost abstractions
- Better than C for large firmware projects

