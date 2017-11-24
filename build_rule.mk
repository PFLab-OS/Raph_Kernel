include common.mk

INCLUDE_PATH = -I$(realpath source/arch/hw/libc/) -I$(realpath source/arch/$(ARCH)/) -I$(realpath source/arch/hw/) -I$(realpath source/kernel)  -I$(realpath source/kernel/freebsd/) -I$(realpath source/kernel/acpica/include/)
RAPHFLAGS = -ggdb3 -O0 \
			-Wall -Werror=return-type -Werror=switch -Werror=unused-result -Wshadow \
			-nostdinc -nostdlib \
			-ffreestanding -fno-builtin \
			-mcmodel=large -mno-mmx \
			-D__KERNEL__ -D_KERNEL -D__NO_LIBC__ \
			-DARCH=$(ARCH) -Dbootverbose=0 $(INCLUDE_PATH) \
			-MMD -MP
RAPHCFLAGS = $(RAPHFLAGS) -std=c99
RAPHCXXFLAGS = $(RAPHFLAGS) -std=c++1y -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-threadsafe-statics
CFLAGS += $(RAPHCFLAGS)
CXXFLAGS += $(RAPHCXXFLAGS)
ASFLAGS += $(RAPHFLAGS)
RAPH_PROJECT_ROOT = $(realpath $(shell pwd))
LANG=C

export CFLAGS
export CXXFLAGS
export ASFLAGS
export RAPH_PROJECT_ROOT
export LANG=C


%:
	$(MAKE) -C source/arch/$(ARCH) -f $(BUILD_RULE_FILE) $@
