CC  = clang
CXX = clang++
FC  = flang
LINKER = $(CC)

ANSI_CFLAGS = -ansi
WARNINGS = -Wno-format -Wall -Wextra

CFLAGS   = -O2 $(WARNINGS) $(ANSI_CFLAGS)
CXXFLAGS = -O2 -std=c++11 $(WARNINGS)
FCFLAGS  = -module ./
LFLAGS   =
DEFINES  = -D_GNU_SOURCE
INCLUDES =
LIBDIRS  =
LIBS     =
