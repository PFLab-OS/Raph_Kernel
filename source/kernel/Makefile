KERNEL = kernel.elf

OBJS = boot/boot.o boot/bootap.o x86_asm.o x86.o main.o apic.o raph_acpi.o gdt.o idt.o \
	 int.o freebsd.o dev.o acpica.o mem/virtmem.o shell.o mem/paging.o mem/physmem.o \
	 mem/dlmalloc.o mem/kstack.o ../libc/lib.o net.o task.o spinlock.o libglobal.o \
	 tty.o queue.o cpu.o elf.o

BUILD = ../../build/
DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

INCLUDE_PATH = -I$(realpath ../libc/) -I$(realpath ./)  -I$(realpath ./freebsd/) -I$(realpath ./acpica/include/)
RAPHFLAGS = -g -Wall -Werror=return-type -Wshadow -nostdinc -nostdlib -fno-builtin -mcmodel=large -D__KERNEL__ -D_KERNEL -Dbootverbose=0 $(INCLUDE_PATH) -MMD -MP
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

dev.o:
	make -C dev CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

net.o:
	make -C net CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

freebsd.o:
	make -C freebsd CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

acpica.o:
	make -C acpica CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)"

../libc/lib.o:
	make -C ../libc CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

clean:
	make -C net clean
	make -C dev clean
	make -C freebsd clean
	make -C acpica clean
	make -C ../libc clean
	-rm -f $(OBJS) $(DEPS) *.s *.ii
