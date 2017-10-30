KERNEL = kernel.elf

SUB = dev freebsd acpica membench #net
OBJS = boot/boot.o boot/bootap.o x86_asm.o x86.o main.o multiboot.o apic.o raph_acpi.o gdt.o idt.o int.o fs.o v6fs.o udp.o mem/virtmem.o shell.o mem/paging.o mem/physmem.o mem/dlmalloc.o mem/kstack.o task.o spinlock.o libglobal.o tty.o queue.o cpu.o elf.o elf_load.o syscall.o syscall_handler.o font.o netbench.o $(addsuffix .o,$(SUB)) arch/hw/libc/lib.o

BUILD = ../../build/
DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

GIT_BRANCH = $(shell git symbolic-ref --short HEAD)

INCLUDE_PATH = -I$(realpath arch/hw/libc/) -I$(realpath arch/$(ARCH)/) -I$(realpath arch/hw/) -I$(realpath ./)  -I$(realpath ./freebsd/) -I$(realpath ./acpica/include/)
RAPHFLAGS = -ggdb3 -O0 -Wall -Werror=return-type -Werror=switch -Werror=unused-result -Wshadow -nostdinc -nostdlib -ffreestanding -fno-builtin -mcmodel=large -mno-mmx -D__KERNEL__ -D_KERNEL -D__NO_LIBC__ -DARCH=$(ARCH) -Dbootverbose=0 $(INCLUDE_PATH) -MMD -MP
RAPHCFLAGS = $(RAPHFLAGS) -std=c99
RAPHCXXFLAGS = $(RAPHFLAGS) -std=c++1y -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-threadsafe-statics
CFLAGS += $(RAPHCFLAGS)
CXXFLAGS += $(RAPHCXXFLAGS)
ASFLAGS += $(RAPHFLAGS)
export CFLAGS
export CXXFLAGS
export ASFLAGS

.PHONY: build clean

default:

build:
	@$(foreach rule, $(SUB), $(MAKE) -C $(rule) build && ):
	@$(MAKE) -C arch/hw/libc
	@$(MAKE) $(BUILD)$(KERNEL)

-include $(DEPS)

$(BUILD)$(KERNEL): $(OBJS) kernel.ld
	$(LD) -nostdinc -nostdlib -Tkernel.ld $(OBJS) -o $@
	@{ echo -n "commit: "; git log -n 1 --format=%H; echo -n "build time: "; TZ='Asia/Tokyo' date "+%Y%m%d-%H%M%S (JST)"; } > $(BUILD)timestamp
	@echo "build information:"
	@cat $(BUILD)timestamp

clean:
	@$(foreach rule, $(SUB), $(MAKE) -C $(rule) clean && ):
	$(MAKE) -C arch/hw/libc clean
	-rm -f $(OBJS) $(DEPS) *.s *.ii
