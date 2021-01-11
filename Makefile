# Inputs intended to be overridden on the make command line
BOARD ?= Relacon_rev1
USB_DESCRIPTORS_VENDOR_ID ?= 0x1209
USB_DESCRIPTORS_PRODUCT_ID ?= 0xfa70
USB_DESCRIPTORS_STRING_SERIAL_NUM ?= A12345
ENABLE_UART_DEBUG ?= 0

# Create ELF, BIN, and (optionally) DFU output files
FIRMWARE_BASENAME := Relacon
FIRMWARE_OUTPUTS := $(addprefix $(FIRMWARE_BASENAME), .elf .bin)
FIRMWARE_OUTPUT_DFU := $(addprefix $(FIRMWARE_BASENAME), .dfu)

# Directory definitions
RELACON_DIR := src
BOARD_DIR := $(RELACON_DIR)/boards/$(BOARD)
STM32F0XX_HAL_DIR := external/stm32f0xx_hal_driver
STM32F0XX_CMSIS_DIR := external/cmsis_device_f0
ARM_CMSIS_DIR := external/CMSIS_5
TINYUSB_DIR := external/tinyusb
PRINTF_DIR := external/printf

# Sources belonging to this repository
RELACON_SRCS := \
	$(wildcard $(RELACON_DIR)/*.c) \
	$(wildcard $(BOARD_DIR)/*.c) \

# Required source files from the STM32F0 HAL framework
STM32F0XX_HAL_SRCS := \
	$(STM32F0XX_HAL_DIR)/Src/stm32f0xx_hal.c \
	$(STM32F0XX_HAL_DIR)/Src/stm32f0xx_hal_cortex.c \
	$(STM32F0XX_HAL_DIR)/Src/stm32f0xx_hal_gpio.c \
	$(STM32F0XX_HAL_DIR)/Src/stm32f0xx_hal_rcc.c \
	$(STM32F0XX_HAL_DIR)/Src/stm32f0xx_hal_rcc_ex.c \
	$(STM32F0XX_HAL_DIR)/Src/stm32f0xx_hal_tim.c \
	$(STM32F0XX_HAL_DIR)/Src/stm32f0xx_hal_uart.c

# Required source files from the CMSIS framework
STM32F0XX_CMSIS_SRCS := \
	$(STM32F0XX_CMSIS_DIR)/Source/Templates/gcc/startup_stm32f042x6.s \
	$(STM32F0XX_CMSIS_DIR)/Source/Templates/system_stm32f0xx.c

# Required source files from the TinyUSB stack
TINYUSB_SRCS := \
	$(TINYUSB_DIR)/src/class/hid/hid_device.c \
	$(TINYUSB_DIR)/src/common/tusb_fifo.c \
	$(TINYUSB_DIR)/src/device/usbd.c \
	$(TINYUSB_DIR)/src/device/usbd_control.c \
	$(TINYUSB_DIR)/src/portable/st/stm32_fsdev/dcd_stm32_fsdev.c \
	$(TINYUSB_DIR)/src/tusb.c

# Lightweight embedded printf implementation (used in debug logging)
PRINTF_SRCS := \
	$(PRINTF_DIR)/printf.c

PRINTF_DEFS := \
	DISABLE_SUPPORT_FLOAT

SRCS := \
	$(RELACON_SRCS) \
	$(STM32F0XX_HAL_SRCS) \
	$(STM32F0XX_CMSIS_SRCS) \
	$(TINYUSB_SRCS)

INCS := \
	$(RELACON_DIR) \
	$(STM32F0XX_HAL_DIR)/Inc \
	$(STM32F0XX_CMSIS_DIR)/Include \
	$(ARM_CMSIS_DIR)/CMSIS/Core/Include \
	$(TINYUSB_DIR)/src

DEFS := \
	STM32F042x6 \
	CFG_TUSB_MCU=OPT_MCU_STM32F0

# Compile in support for UART debug logging if selected
ifeq ($(ENABLE_UART_DEBUG),1)
	DEFS += ENABLE_UART_DEBUG

	SRCS += $(PRINTF_SRCS)
	DEFS += $(PRINTF_DEFS)
	INCS += $(PRINTF_DIR)
endif

OBJS := $(filter %.o,$(SRCS:.c=.o) $(SRCS:.s=.o))

DEPS := $(filter %.d,$(SRCS:.c=.d))

# Pass in the USB descriptor definitions from this Makefile when compiling the
# USB descriptors source file
CFLAGS_USB_DESCRIPTORS := \
	-DUSB_DESCRIPTORS_VENDOR_ID=$(USB_DESCRIPTORS_VENDOR_ID) \
	-DUSB_DESCRIPTORS_PRODUCT_ID=$(USB_DESCRIPTORS_PRODUCT_ID) \
	-D'USB_DESCRIPTORS_STRING_SERIAL_NUM="$(USB_DESCRIPTORS_STRING_SERIAL_NUM)"'
$(RELACON_DIR)/UsbDescriptors.o: CFLAGS += $(CFLAGS_USB_DESCRIPTORS)

# Use the ARM bare metal toolchain (must be in the PATH)
PREFIX := arm-none-eabi-
AS := $(PREFIX)as
CC := $(PREFIX)gcc
LD := $(PREFIX)ld
OBJCOPY := $(PREFIX)objcopy

# Small Python utility for creating a ".dfu" file appropriate for use with the
# DfuSe programming tool
MKDFU := tools/mkdfu.py

CFLAGS := \
	-mthumb \
	-mcpu=cortex-m0 \
	-Os \
	-ffunction-sections \
	-fdata-sections \
	-Wall \
	-Wshadow \
	-Wundef \
	-MD \
	$(addprefix -I,$(INCS)) \
	$(addprefix -D,$(DEFS))

LDFLAGS := \
	-Wl,--gc-sections \
	-T $(BOARD_DIR)/main.ld

# Default rule. Build all the firmware outputs
.PHONY: all
all: $(FIRMWARE_OUTPUTS)

# Rule to clean up the files generated during the build
.PHONY: clean
clean:
	rm -fv $(OBJS) $(DEPS) $(FIRMWARE_OUTPUTS) $(FIRMWARE_OUTPUT_DFU)

# Keep the object files around for quick rebuilds
.PRECIOUS: $(OBJS) $(DEPS)

# Link all of the object files into an ELF executable
%.elf: $(OBJS)
	$(LINK.c) $^ $(LDLIBS) -o $@

# Transform an ELF executable into a flat binary image
%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

# Transform a binary image into a ST DFU file
%.dfu: %.bin $(MKDFU)
	$(MKDFU) 0x08000000 $< $@

# Rule to program the device using the ST DfuSe software (must be in PATH)
.PHONY: program
program: $(FIRMWARE_OUTPUT_DFU)
	dfu-util -a 0 -D $<

# Include automatically-generated header dependency rules
-include $(DEPS)