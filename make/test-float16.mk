ifeq ($(findstring $(MAKECMDGOALS),clean),)
TEST_FLOAT16 := $(shell $(CC) $(MAKE_DIR)/test-float16.c -o /dev/null 1>/dev/null 2>&1; echo $$?)
ifeq ($(strip $(TEST_FLOAT16)),0)
DEFINES += -DWITH_HALF_PRECISION
else
$(info Compiler $(CC) does not support half precision data types)
endif
endif
