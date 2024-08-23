# Project Name
TARGET = Daisy_Gran

USE_FATFS = 1
# USE_DAISYSP_LGPL = 1
# Sources
CPP_SOURCES = main.cpp AudioFileManager.cpp GranularSynth.cpp\
							GrannyChordApp.cpp Grain.cpp ChordMode.cpp\
							DaisySP-LGPL-FX/compressor.cpp DaisySP-LGPL-FX/moogladder.cpp\
							DaisySP-LGPL-FX/reverb.cpp

# C_INCLUDES += -I.../DaisySP/DaisySP-LGPL/Source

# Library locations
LIBDAISY_DIR = ../libDaisy
DAISYSP_DIR = ../DaisySP

# APP_TYPE = BOOT_SRAM
APP_TYPE = BOOT_SRAM_EDITED

# Core location, and generic Makefile
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

LDFLAGS += -u _printf_float -Wl,-Map=output.map
# OPT += -Os
OPT += -O0
CFLAGS += -DDEBUG_MODE=1 -ffunction-sections -fdata-sections
