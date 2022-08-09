#################################################################
#################################################################
# Configuration options                                         #
#################################################################
#################################################################

# Please have a look in INSTALL and the WIKI for details on
# configuration options setup steps.
# supported: GCC, CLANG, ICC, MIC (ICC), GCCX86 (for 32bit systems)
COMPILER = GCC#NO SPACE

# Path were to install likwid
PREFIX ?= /usr/local#NO SPACE

# LIKWID configuration
LIKWID_LIBPATH=/usr/local/lib
LIKWID_INCPATH=/usr/local/inc

#################################################################
#################################################################
# Advanced configuration options                                #
# Most users do not need to change values below this comment!   #
#################################################################
#################################################################

# Define the color of the likwid-pin output
# Can be NONE, BLACK, RED, GREEN, YELLOW, BLUE,
# MAGENTA, CYAN or WHITE
COLOR = BLUE#NO SPACE

# Some path definitions
MANPREFIX = $(PREFIX)/man#NO SPACE
BINPREFIX = $(PREFIX)/bin#NO SPACE
LIBPREFIX = $(PREFIX)/lib#NO SPACE

# These paths are hardcoded into executables and libraries. Usually
# they'll be the same as above, but package maintainers may want to
# distinguish between the image directories and the final install
# target.
# Keep in mind that the access and setFreq daemon need enough
# privileges that may be deleted when copying the files to
# the INTSTALLED_PREFIX
INSTALLED_PREFIX ?= $(PREFIX)#NO SPACE
INSTALLED_BINPREFIX = $(INSTALLED_PREFIX)/bin#NO SPACE
INSTALLED_LIBPREFIX = $(INSTALLED_PREFIX)/lib#NO SPACE

# Build LIKWID with debug flags
DEBUG = true#NO SPACE

# Versioning Information
# The libraries are named liblikwid.<VERSION>.<RELEASE>
VERSION = 0
RELEASE = 1
MINOR = 1
# Date when the release is published
DATE    = 23.06.2019

# In come cases it is important to set the rpaths for the LIKWID library. One
# example is the use of sudo because it resets environment variables like
# LD_LIBRARY_PATH
RPATHS = -Wl,-rpath=$(INSTALLED_LIBPREFIX)

ASM_BASE = ./asm_base

JSMN_PATH = ./jsmn

ADDRESS_SANITIZER = true
