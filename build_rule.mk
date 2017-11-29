include common.mk

INCLUDE_PATH = -I$(realpath source/arch/hw/) -I$(realpath source/arch/$(ARCH)/) -I$(realpath source/kernel)
KERNEL_INCLUDE_PATH = -I$(realpath source/arch/hw/libc/) -I$(realpath source/kernel/freebsd/) -I$(realpath source/kernel/acpica/include/)
RAPH_FLAGS = -ggdb3 -O0 \
			-Wall -Werror=return-type -Werror=switch -Werror=unused-result -Wshadow \
			-DARCH=$(ARCH) $(INCLUDE_PATH) \
			-MMD -MP
RAPH_KERNELFLAGS = -nostdinc -nostdlib \
			-ffreestanding -fno-builtin \
			-mcmodel=large -mno-mmx \
			-D__KERNEL__ -D_KERNEL -D__NO_LIBC__ \
			-DARCH=$(ARCH) -Dbootverbose=0 \
			$(KERNEL_INCLUDE_PATH)
RAPH_CFLAGS = $(RAPH_FLAGS) -std=c99
RAPH_CXXFLAGS = $(RAPH_FLAGS) -std=c++1y
RAPH_KERNELCFLAGS = $(RAPH_KERNELFLAGS) $(RAPH_CFLAGS)
RAPH_KERNELCXXFLAGS = $(RAPH_KERNELFLAGS) $(RAPH_CXXFLAGS) -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-threadsafe-statics
CFLAGS += $(RAPH_KERNELCFLAGS)
CXXFLAGS += $(RAPH_KERNELCXXFLAGS)
ASFLAGS += $(RAPH_FLAGS) $(RAPH_KERNELFLAGS)
RAPH_PROJECT_ROOT = $(realpath $(shell pwd))
LANG=C

export RAPH_CFLAGS
export RAPH_CXXFLAGS
export CFLAGS
export CXXFLAGS
export ASFLAGS
export RAPH_PROJECT_ROOT
export LANG=C

test:
	$(MAKE) -C source/unit_test test

clean:
	$(MAKE) -C source/unit_test clean
	$(MAKE) -C source/arch/$(ARCH) -f $(BUILD_RULE_FILE) clean

%:
	$(MAKE) -C source/arch/$(ARCH) -f $(BUILD_RULE_FILE) $@
