CC  = gcc
CXX = g++
FC  = gfortran
LINKER = $(CC)

ANSI_CFLAGS  = -ansi
WARNINGS = #-Wno-format -Wall -Wextra

CFLAGS   = -std=c99 -O2 -fverbose-asm $(WARNINGS)
CXXFLAGS = -O2 -std=c++11 $(WARNINGS)
FCFLAGS  = -J ./  -fsyntax-only
LFLAGS   =
DEFINES  = -D_GNU_SOURCE
INCLUDES =
LIBDIRS  =
LIBS     =

ifeq ($(strip $(ADDRESS_SANITIZER)), true)
CPPFLAGS += -fsanitize=address -fsanitize-address-use-after-scope -fno-omit-frame-pointer
LIBS     += -lasan
endif
