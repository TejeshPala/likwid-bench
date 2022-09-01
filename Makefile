

#CONFIGURE BUILD SYSTEM
include config.mk
TARGET	   = likwid-bench
all: $(TARGET)
SRC_DIR    = ./src
INC_DIR    = ./include
MAKE_DIR   = ./make
Q         ?= @

#DO NOT EDIT BELOW
include $(MAKE_DIR)/include_$(strip $(COMPILER)).mk
INCLUDES  += -g -I$(INC_DIR) -I$(BUILD_DIR)
LIBDIRS   += 
LIBS      += -lm  -ldl
LFLAGS    += -pthread
DEFINES   += -DWITH_BSTRING -DCALCULATOR_AS_LIB -DLIKWIDBENCH_KERNEL_FOLDER=$(LIKWIDBENCH_KERNEL_FOLDER)

VPATH     = $(SRC_DIR)
ASM       = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.s,$(wildcard $(SRC_DIR)/*.c))
OBJ       = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o,$(wildcard $(SRC_DIR)/*.c))


include ./ext/Makefile.bstrlib_helper
include ./ext/Makefile.calculator
include ./ext/Makefile.cmap


CPPFLAGS := $(CPPFLAGS) $(DEFINES) $(INCLUDES)

$(TARGET): $(BUILD_DIR) $(OBJ) likwid-bench.c
	@echo "===>  LINKING  $(TARGET)"
	$(Q)$(LINKER) $(DEFINES) $(INCLUDES) $(LFLAGS) $(LIBDIRS) $(OBJ) likwid-bench.c -o $(TARGET) $(LIBS)

asm:  $(BUILD_DIR) $(ASM)

update_submodules:
	$(Q)git submodule update --recursive

$(BUILD_DIR)/%.o:  %.c
	@echo "===>  COMPILE  $@"
	$(Q)$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@
	$(Q)$(CC) $(CPPFLAGS) $(CFLAGS) -MT $(@:.d=.o) -MM  $< > $(BUILD_DIR)/$*.d

$(BUILD_DIR)/%.s:  %.c
	@echo "===>  GENERATE ASM  $@"
	$(Q)$(CC) -S $(CPPFLAGS) $(CFLAGS) $< -o $@

tags:
	@echo "===>  GENERATE  TAGS"
	$(Q)ctags -R

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

test:
	make -C tests run

ifeq ($(findstring $(MAKECMDGOALS),clean),)
-include $(OBJ:.o=.d)
endif

.PHONY: clean distclean

clean:
	@echo "===> CLEAN"
	@make --no-print-directory -C $(BSTRLIB_HELPER_DIR) clean
	@make --no-print-directory -C $(CALCULATOR_DIR) clean
	@rm -rf $(BUILD_DIR)
	@rm -f tags

distclean: clean
	@echo "===> DIST CLEAN"
	@rm -f $(TARGET)
	@rm -f tags

