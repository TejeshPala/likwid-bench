#config-checks.mk

ifeq ($(findstring $(MAKECMDGOALS),clean),)
GLIBC_VERSION := $(shell $(CC) $(MAKE_DIR)/check-pthread-setaffinity-np.c -o /tmp/test_glibc 2>/dev/null && /tmp/test_glibc && rm -f /tmp/test_glibc)

HAS_SCHEDAFFINITY := $(shell if [ "$(GLIBC_VERSION)" = "$$(echo -e "$(GLIBC_VERSION)\n2.4" | sort -V | head -n1)" ]; then echo 0; else echo 1; fi)

ifeq ($(strip $(HAS_SCHEDAFFINITY)),1)
DEFINES += -DHAS_SCHEDAFFINITY
$(info GLIBC version $(GLIBC_VERSION) supports pthread_setaffinity_np!);
else
$(info GLIBC version $(GLIBC_VERSION) has no pthread_setaffinity_np support!);
endif

endif
