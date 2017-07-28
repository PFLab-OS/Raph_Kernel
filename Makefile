ARCH ?= hw/x86
export ARCH

default:
	$(MAKE) -C source/kernel/arch/$(ARCH)

%:
	$(MAKE) -C source/kernel/arch/$(ARCH) $@
