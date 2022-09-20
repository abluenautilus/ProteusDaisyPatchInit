# Project Name
TARGET = ProteusDaisy

# Sources
CPP_SOURCES = ProteusDaisy.cpp
#DEBUG=1

# Library Locations
LIBDAISY_DIR = /Users/jonask/Music/Daisy/DaisyExamples/libDaisy
DAISYSP_DIR = /Users/jonask/Music/Daisy/DaisyExamples/DaisySP

# Linker flags
# This is not really required, used only for profiling! Increases executable size by ~8kB
LDFLAGS = -u _printf_float

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

