# Project Settings
PROJECT = firmware
TARGET  = $(PROJECT).elf
BIN     = $(PROJECT).bin
HEX     = $(PROJECT).hex

# Toolchain
PREFIX  = arm-none-eabi-
CC      = $(PREFIX)gcc
CXX     = $(PREFIX)g++
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
SIZE    = $(PREFIX)size

# MCU Settings
MCU       = cortex-m4
FPU       = fpv4-sp-d16
FLOAT_ABI = hard

# --- DIRECTORIES ---
PLATFORM_DIR = ../platform
DRIVERS_DIR  = $(PLATFORM_DIR)/drivers
NO_OS_DIR    = $(PLATFORM_DIR)/no-os_files
API_DIR      = VL53L0X_API
BUILD_DIR    = build

# --- FILES ---
STARTUP_FILE = $(PLATFORM_DIR)/startup_stm32f411xe.s
LDSCRIPT     = $(PLATFORM_DIR)/STM32F411CEU6_FLASH.ld

# 1. Manually listed objects
OBJ_FILES = $(BUILD_DIR)/main.o \
            $(BUILD_DIR)/system_stm32f4xx.o \
            $(BUILD_DIR)/segger_fault_handler.o \
            $(BUILD_DIR)/syscalls.o \
            $(BUILD_DIR)/sysmem.o \
            $(BUILD_DIR)/startup.o

# 2. Automatically find VL53L0X API C files
API_SOURCES = $(wildcard $(API_DIR)/*.c)
API_OBJS    = $(patsubst $(API_DIR)/%.c, $(BUILD_DIR)/%.o, $(API_SOURCES))

# Combine all objects
ALL_OBJS = $(OBJ_FILES) $(API_OBJS)

# Flags
CFLAGS = -mcpu=$(MCU) -mthumb -mfpu=$(FPU) -mfloat-abi=$(FLOAT_ABI) \
         -Og -g3 -Wall -fdata-sections -ffunction-sections \
         -DSTM32F411xE \
         -I$(DRIVERS_DIR) -I$(NO_OS_DIR) -I$(API_DIR)

# API specific flags to silence warnings and handle string functions
CFLAGS += -DVL53L0X_LOG_ENABLE=0 \
          -DLOG_FUNCTION_START\(fmt,...\)= \
          -DLOG_FUNCTION_END\(status,...\)= \
          -DLOG_FUNCTION_END_FMT\(status,fmt,...\)= \
          -include string.h \
          -DVL53L0X_COPYSTRING=strcpy

LDFLAGS = -mcpu=$(MCU) -mthumb -mfpu=$(FPU) -mfloat-abi=$(FLOAT_ABI) \
          -Wl,--gc-sections -T$(LDSCRIPT) \
          -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map \
          -specs=nano.specs \
          -Wl,--no-warn-rwx-segments

# --- Rules ---
all: $(BUILD_DIR)/$(TARGET) $(BUILD_DIR)/$(BIN) $(BUILD_DIR)/$(HEX)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Compile main.cpp
$(BUILD_DIR)/main.o: main.cpp | $(BUILD_DIR)
	@echo "Compiling C++ Source: $<"
	@$(CXX) $(CFLAGS) -c $< -o $@

# Compile the VL53L0X API files
$(BUILD_DIR)/%.o: $(API_DIR)/%.c | $(BUILD_DIR)
	@echo "Compiling API File: $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Platform files
$(BUILD_DIR)/syscalls.o: $(NO_OS_DIR)/syscalls.c | $(BUILD_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/sysmem.o: $(NO_OS_DIR)/sysmem.c | $(BUILD_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/system_stm32f4xx.o: $(DRIVERS_DIR)/system_stm32f4xx.c | $(BUILD_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/segger_fault_handler.o: $(DRIVERS_DIR)/segger_fault_handler.c | $(BUILD_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/startup.o: $(STARTUP_FILE) | $(BUILD_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@

# Link all objects
$(BUILD_DIR)/$(TARGET): $(ALL_OBJS)
	@echo "Linking: $@"
	@$(CXX) $(ALL_OBJS) $(LDFLAGS) -o $@
	@$(SIZE) $@

$(BUILD_DIR)/$(BIN): $(BUILD_DIR)/$(TARGET)
	@$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/$(HEX): $(BUILD_DIR)/$(TARGET)
	@$(OBJCOPY) -O ihex $< $@

# --- Utility Commands ---

dfu: $(BUILD_DIR)/$(BIN)
	@echo "Flashing via DFU..."
	dfu-util -a 0 -s 0x08000000:leave -D $(BUILD_DIR)/$(BIN)

openocd_flash: $(BUILD_DIR)/$(BIN)
	@echo "Flashing via OpenOCD..."
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program $< verify reset exit 0x08000000"
	@echo "✅ Flashing Done Successfully"

clean:
	@echo "Cleaning build directory..."
	@rm -rf $(BUILD_DIR)

.PHONY: all clean dfu openocd_flash
