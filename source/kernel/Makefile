KERNEL = kernel.elf

OBJS = boot/boot.o boot/bootap.o x86_asm.o x86.o main.o multiboot.o apic.o raph_acpi.o gdt.o idt.o int.o freebsd.o dev.o acpica.o mem/virtmem.o shell.o mem/paging.o mem/physmem.o mem/dlmalloc.o mem/kstack.o ../libc/lib.o task.o spinlock.o libglobal.o tty.o queue.o cpu.o membench.o netbench.o # net.o

BUILD = ../../build/
DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

GIT_BRANCH = $(shell git symbolic-ref --short HEAD)

ifeq ($(GIT_BRANCH),xeonphi)
	FLAGS = -DFLAG_MEMBENCH
endif

INCLUDE_PATH = -I$(realpath ../libc/) -I$(realpath ./)  -I$(realpath ./freebsd/) -I$(realpath ./acpica/include/)
RAPHFLAGS = -g -Wall -Werror=return-type -Wshadow -nostdinc -nostdlib -fno-builtin -mcmodel=large -D__KERNEL__ -D_KERNEL -Dbootverbose=0 $(FLAGS) $(INCLUDE_PATH) -MMD -MP
RAPHCFLAGS = $(RAPHFLAGS) -std=c99
RAPHCXXFLAGS = $(RAPHFLAGS) -std=c++1y -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-threadsafe-statics
CFLAGS += $(RAPHCFLAGS)
CXXFLAGS += $(RAPHCXXFLAGS)

ASFLAGS += $(RAPHFLAGS)

.PHONY: clean dev.o net.o freebsd.o acpica.o ../libc/lib.o

default: $(BUILD)$(KERNEL)

-include $(DEPS)

$(BUILD)$(KERNEL): $(OBJS) kernel.ld
	$(LD) -nostdinc -nostdlib -Tkernel.ld $(OBJS) -o $@
	@{ echo -n "commit: "; git log -n 1 --format=%H; echo -n "build time: "; TZ='Asia/Tokyo' date "+%Y%m%d-%H%M%S (JST)"; } > $(BUILD)timestamp
	@echo "build information:"
	@cat $(BUILD)timestamp

dev.o:
	@$(MAKE) -C dev CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

net.o:
	@$(MAKE) -C net CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

freebsd.o:
	@$(MAKE) -C freebsd CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

acpica.o:
	@$(MAKE) -C acpica CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)"

../libc/lib.o:
	@$(MAKE) -C ../libc CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

clean:
	$(MAKE) -C net clean
	$(MAKE) -C dev clean
	$(MAKE) -C freebsd clean
	$(MAKE) -C acpica clean
	$(MAKE) -C ../libc clean
	-rm -f $(OBJS) $(DEPS) *.s *.ii
