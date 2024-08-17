# Project Name
TARGET = Daisy_Gran

# This is necessary if we want to use FatFS in our code
USE_FATFS = 1
USE_DAISYSP_LGPL = 1
# Sources
CPP_SOURCES = main.cpp AudioFileManager.cpp GranularSynth.cpp GrannyChordApp.cpp
# C_INCLUDES += -I.../DaisySP/DaisySP-LGPL/Source

# Library locations
LIBDAISY_DIR = ../libDaisy
DAISYSP_DIR = ../DaisySP
APP_TYPE = BOOT_SRAM

# Core location, and generic Makefile
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

LDFLAGS += -u _printf_float
OPT += -Os
CFLAGS += -DDEBUG_MODE=1 
# -ffunction-sections -fdata-sections -flto 

