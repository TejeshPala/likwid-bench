CC  = icc
CXX = icpc
FC  = ifort
LINKER = $(CC)

ANSI_CFLAGS  = -ansi
WARNINGS = -Wno-format -Wall -Wextra

CFLAGS   = -std=c99 -O2 -fverbose-asm $(ANSI_CFLAGS) $(WARNINGS)
CXXFLAGS = -O2 -std=c++11 $(WARNINGS)
FCFLAGS  = -module ./
LFLAGS   =
DEFINES  = -D_GNU_SOURCE
INCLUDES =
LIBDIRS  =
LIBS     =


