TAG ?= GCC
APPLICATION=bench

#CONFIGURE BUILD SYSTEM
TARGET	   = $(APPLICATION)-$(TAG)
BUILD_DIR  = ./$(TAG)
SRC_DIR    = ./src
INC_DIR    = ./include
MAKE_DIR   = ./make
Q         ?= @

#DO NOT EDIT BELOW
include $(MAKE_DIR)/include_$(TAG).mk
INCLUDES  += -g -I$(INC_DIR) -I$(LIKWID_INCDIR)
LIBDIRS   += -L$(LIKWID_LIBDIR)
LIBS      += -lm  -ldl
LFLAGS    += -pthread -DCALCULATOR_AS_LIB
DEFINES   += -DWITH_BSTRING -DCALCULATOR_AS_LIB -DLIKWIDBENCH_KERNEL_FOLDER=$(shell pwd)/kernels

BSTRLIB_DIR := ext/bstrlib_helper/ext/bstrlib
BSTRLIB_HELPER_DIR := ext/bstrlib_helper
CMAP_DIR := ext/cmap
CMAP_SRC_DIR := ext/cmap/src
CMAP_INC_DIR := ext/cmap/include
GETOPT_DIR := ext/getopt_extra
CALCULATOR_DIR := ext/calculator
INCLUDES += -I$(BSTRLIB_DIR) -I$(BSTRLIB_HELPER_DIR) -I$(CMAP_INC_DIR) -I$(GETOPT_DIR) -I$(CALCULATOR_DIR)

VPATH     = $(SRC_DIR)
ASM       = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.s,$(wildcard $(SRC_DIR)/*.c))
OBJ       = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o,$(wildcard $(SRC_DIR)/*.c))
OBJ      += $(patsubst $(SRC_DIR)/%.cc, $(BUILD_DIR)/%.o,$(wildcard $(SRC_DIR)/*.cc))
OBJ      += $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o,$(wildcard $(SRC_DIR)/*.cpp))
OBJ      += $(patsubst $(SRC_DIR)/%.f90, $(BUILD_DIR)/%.o,$(wildcard $(SRC_DIR)/*.f90))
OBJ      += $(patsubst $(SRC_DIR)/%.F90, $(BUILD_DIR)/%.o,$(wildcard $(SRC_DIR)/*.F90))
CPPFLAGS := $(CPPFLAGS) $(DEFINES) $(INCLUDES)

OBJ      += $(patsubst $(BSTRLIB_DIR)/%.c, $(BUILD_DIR)/%.o,$(wildcard $(BSTRLIB_DIR)/bstrlib.c))
OBJ      += $(patsubst $(BSTRLIB_HELPER_DIR)/%.c, $(BUILD_DIR)/%.o,$(wildcard $(BSTRLIB_HELPER_DIR)/bstrlib_helper.c))
OBJ      += $(patsubst $(CMAP_SRC_DIR)/%.c, $(BUILD_DIR)/%.o,$(wildcard $(CMAP_SRC_DIR)/map.c))
OBJ      += $(patsubst $(CMAP_SRC_DIR)/%.c, $(BUILD_DIR)/%.o,$(wildcard $(CMAP_SRC_DIR)/ghash.c))
OBJ      += $(patsubst $(GETOPT_DIR)/%.c, $(BUILD_DIR)/%.o,$(wildcard $(GETOPT_DIR)/getopt_extra.c))
OBJ      += $(patsubst $(CALCULATOR_DIR)/%.c, $(BUILD_DIR)/%.o,$(wildcard $(CALCULATOR_DIR)/calculator.c))
OBJ      += $(patsubst $(CALCULATOR_DIR)/%.c, $(BUILD_DIR)/%.o,$(wildcard $(CALCULATOR_DIR)/stack.c))

$(TARGET): $(BUILD_DIR) $(OBJ) likwid-bench.c
	@echo "===>  LINKING  $(TARGET)"
	$(Q)$(LINKER) $(DEFINES) $(INCLUDES) $(LFLAGS) $(LIBDIRS) $(OBJ) likwid-bench.c -o $(TARGET) $(LIBS)

asm:  $(BUILD_DIR) $(ASM)

$(BSTRLIB_HELPER_DIR)/bstrlib_helper.c:
	$(Q)git submodule update --init --recursive

$(BUILD_DIR)/bstrlib.o: $(BSTRLIB_DIR)/bstrlib.c 
	@echo "===>  COMPILE  $@"
	$(Q)make -s --no-print-directory -C $(BSTRLIB_DIR) bstrlib.o
	$(Q)mv $(BSTRLIB_DIR)/bstrlib.o $(BUILD_DIR)/bstrlib.o

$(BUILD_DIR)/bstrlib_helper.o: $(BSTRLIB_HELPER_DIR)/bstrlib_helper.c
	@echo "===>  COMPILE  $@"
	$(Q)make -s --no-print-directory -C $(BSTRLIB_HELPER_DIR) bstrlib_helper.o
	$(Q)mv $(BSTRLIB_HELPER_DIR)/bstrlib_helper.o $(BUILD_DIR)/bstrlib_helper.o

$(BUILD_DIR)/map.o:  $(CMAP_SRC_DIR)/map.c
	$(Q)git submodule update --init --recursive
	@echo "===>  COMPILE  $@"
	$(Q)$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

$(BUILD_DIR)/ghash.o:  $(CMAP_SRC_DIR)/ghash.c
	$(Q)git submodule update --init --recursive
	@echo "===>  COMPILE  $@"
	$(Q)$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

$(GETOPT_DIR)/getopt_extra.c:
	$(Q)git submodule update --init --recursive

$(BUILD_DIR)/getopt_extra.o: $(GETOPT_DIR)/getopt_extra.c
	@echo "===>  COMPILE  $@"
	$(Q)make -s --no-print-directory -C $(GETOPT_DIR) getopt_extra.o
	$(Q)mv $(GETOPT_DIR)/getopt_extra.o $(BUILD_DIR)/getopt_extra.o

$(CALCULATOR_DIR)/calculator.c:
	$(Q)git submodule update --init --recursive

$(BUILD_DIR)/calculator.o: $(CALCULATOR_DIR)/calculator.c
	@echo "===>  COMPILE  $@"
	$(Q)make --no-print-directory -C $(CALCULATOR_DIR) calculator.o
	$(Q)mv $(CALCULATOR_DIR)/calculator.o $(BUILD_DIR)/calculator.o

$(BUILD_DIR)/stack.o: $(CALCULATOR_DIR)/stack.c
	@echo "===>  COMPILE  $@"
	$(Q)make --no-print-directory -C $(CALCULATOR_DIR) stack.o
	$(Q)mv $(CALCULATOR_DIR)/stack.o $(BUILD_DIR)/stack.o

$(BUILD_DIR)/%.o:  %.c
	@echo "===>  COMPILE  $@"
	$(Q)$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@
	$(Q)$(CC) $(CPPFLAGS) $(CFLAGS) -MT $(@:.d=.o) -MM  $< > $(BUILD_DIR)/$*.d

$(BUILD_DIR)/%.s:  %.c
	@echo "===>  GENERATE ASM  $@"
	$(Q)$(CC) -S $(CPPFLAGS) $(CFLAGS) $< -o $@

#$(BUILD_DIR)/%.o:  %.cc
#	@echo "===>  COMPILE  $@"
#	$(Q)$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@
#	$(Q)$(CXX) $(CPPFLAGS) -MT $(@:.d=.o) -MM  $< > $(BUILD_DIR)/$*.d

#$(BUILD_DIR)/%.o:  %.cpp
#	@echo "===>  COMPILE  $@"
#	$(Q)$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@
#	$(Q)$(CXX) $(CPPFLAGS) -MT $(@:.d=.o) -MM  $< > $(BUILD_DIR)/$*.d

#$(BUILD_DIR)/%.o:  %.f90
#	@echo "===>  COMPILE  $@"
#	$(Q)$(FC) -c  $(FCFLAGS) $< -o $@

#$(BUILD_DIR)/%.o:  %.F90
#	@echo "===>  COMPILE  $@"
#	$(Q)$(FC) -c  $(CPPFLAGS)  $(FCFLAGS) $< -o $@

tags:
	@echo "===>  GENERATE  TAGS"
	$(Q)ctags -R

$(BUILD_DIR):
	@mkdir $(BUILD_DIR)

test:
	make -C tests run

ifeq ($(findstring $(MAKECMDGOALS),clean),)
-include $(OBJ:.o=.d)
endif

.PHONY: clean distclean

clean:
	@echo "===> CLEAN"
	@make --no-print-directory -C $(GETOPT_DIR) clean
	@make --no-print-directory -C $(BSTRLIB_HELPER_DIR) clean
	@make --no-print-directory -C $(CALCULATOR_DIR) clean
	@rm -rf $(BUILD_DIR)
	@rm -f tags

distclean: clean
	@echo "===> DIST CLEAN"
	@rm -f $(TARGET)
	@rm -f tags

