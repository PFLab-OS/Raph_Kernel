KERNEL = kernel.elf

SUB = dev freebsd acpica membench net
OBJS = boot/boot.o boot/bootap.o x86_asm.o x86.o main.o multiboot.o apic.o raph_acpi.o gdt.o idt.o int.o fs.o v6fs.o mem/virtmem.o shell.o mem/paging.o mem/physmem.o mem/dlmalloc.o mem/kstack.o thread.o spinlock.o libglobal.o tty.o queue.o cpu.o elf.o elf_load.o syscall.o syscall_handler.o font.o netbench.o $(addsuffix .o,$(SUB)) ../arch/hw/libc/lib.o

BUILD = $(RAPH_PROJECT_ROOT)/build/
DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: build clean

include $(RAPH_PROJECT_ROOT)/common_rules.mk

default:

build:
	@$(foreach rule, $(SUB), $(MAKE) -C $(rule) build && ):
	@$(MAKE) $(BUILD)$(KERNEL)

-include $(DEPS)

$(BUILD)$(KERNEL): $(OBJS) kernel.ld
	@echo "Linking kernel..."
	@$(LD) -nostdinc -nostdlib -Tkernel.ld $(OBJS) -o $@
	@{ echo -n "commit: "; git log -n 1 --format=%H; echo -n "build time: "; TZ='Asia/Tokyo' date "+%Y%m%d-%H%M%S (JST)"; } > $(BUILD)timestamp
	@echo "build information:"
	@cat $(BUILD)timestamp

clean:
	@$(foreach rule, $(SUB), $(MAKE) -C $(rule) clean && ):
	-rm -f $(OBJS) $(DEPS) *.s *.ii

.syntastic_cpp_config: Makefile
	echo $(RAPHCXXFLAGS) > $@
