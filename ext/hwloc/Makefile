SRC_DIRS    = ./hwloc
MAKE_DIR   = ../../make

#DO NOT EDIT BELOW

include ../../config.mk
include $(MAKE_DIR)/include_$(strip $(COMPILER)).mk

CFLAGS    += -O2 -fPIC -fvisibility=hidden
INCLUDES  += -I./include
#DEFINES   =
LIBS      += -L. -lm
LFLAGS    += -fPIC -fvisibility=hidden
Q         ?= @
DEFINES := $(filter-out -DVERSION=$(VERSION),$(DEFINES)) -DRUNSTATEDIR=\"/var/run\"
ifeq ($(DEBUG),true)
DEBUG_FLAGS = -g
else
DEBUG_FLAGS =
endif
ifeq ($(COMPILER),MIC)
CFLAGS += -mmic
LFLAGS += -mmic
endif


#CONFIGURE BUILD SYSTEM
BUILD_DIR  = ./$(COMPILER)

VPATH     = $(SRC_DIRS)
FILES     = $(notdir $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.c)))
OBJ       = $(patsubst %.c, $(BUILD_DIR)/%.o, $(FILES))
#LIBHWLOC = $(shell basename $(TARGET_HWLOC_LIB))
ifneq ($(filter GCCARMv7 GCCARMv8 GCCARM ARMCLANG FCC GCCPOWER XLC,$(COMPILER)),)
OBJ := $(filter-out $(BUILD_DIR)/topology-x86.o, $(OBJ))
endif

EXCLUDE_TOPOLOGIES := freebsd pci xml-libxml netbsd windows darwin gl solaris solaris-chiptype nvml hpux cuda opencl aix levelzero rsmi
EXCLUDE_OBJ := $(addprefix $(BUILD_DIR)/topology-,$(addsuffix .o,$(EXCLUDE_TOPOLOGIES)))
OBJ := $(filter-out $(EXCLUDE_OBJ),$(OBJ))

CPPFLAGS := $(CPPFLAGS) $(DEFINES) $(INCLUDES)

.NOTPARALLEL: all

all: $(BUILD_DIR) $(OBJ) $(STATIC_LIBHWLOC) $(SHARED_LIBHWLOC)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(STATIC_LIBHWLOC): $(OBJ)
	@echo "===> STATIC LIB $(STATIC_LIBHWLOC)"
	$(Q)${AR} -crs $(STATIC_LIBHWLOC) $(OBJ)

$(SHARED_LIBHWLOC): $(OBJ)
	@echo "===> SHARED LIB $(SHARED_LIBHWLOC)"
	$(Q)$(CC) $(DEBUG_FLAGS) $(LFLAGS) -Wl,-soname,$(SHARED_LIBHWLOC).$(VERSION).$(RELEASE)  -Wall -shared -fPIC -o $(SHARED_LIBHWLOC) $(OBJ) $(LIBS) $(RPATHS)
	@ln -sf $(SHARED_LIBHWLOC) $(SHARED_LIBHWLOC).$(VERSION).$(RELEASE)
	@ln -sf $(SHARED_LIBHWLOC) $(SHARED_LIBHWLOC).$(VERSION)

#PATTERN RULES
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@echo "===> COMPILING $<"
	$(Q)$(CC) -c $(DEBUG_FLAGS) $(CFLAGS) $(CPPFLAGS) $< -o $@
	$(Q)$(CC) $(DEBUG_FLAGS) $(CPPFLAGS) -MT $(@:.d=.o) -MM  $< > $(BUILD_DIR)/$*.d

ifeq ($(findstring $(MAKECMDGOALS),clean),)
-include $(OBJ:.o=.d)
endif

.PHONY: clean distclean

clean:
	@echo "===> CLEAN"
	@rm -f $(SHARED_LIBHWLOC) $(STATIC_LIBHWLOC) $(SHARED_LIBHWLOC).$(VERSION).$(RELEASE) $(SHARED_LIBHWLOC).$(VERSION)

distclean: clean
	@echo "===> DIST CLEAN"
	@rm -rf $(BUILD_DIR)
