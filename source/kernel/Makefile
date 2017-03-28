KERNEL = kernel.elf

SUB = dev freebsd acpica #net
OBJS = boot/boot.o boot/bootap.o x86_asm.o x86.o main.o multiboot.o apic.o raph_acpi.o gdt.o idt.o int.o mem/virtmem.o shell.o mem/paging.o mem/physmem.o mem/dlmalloc.o mem/kstack.o task.o spinlock.o libglobal.o tty.o queue.o cpu.o elf.o elf_load.o syscall.o syscall_handler.o font.o membench/membench.o membench/test2.o membench/test3.o membench/test4.o membench/test5.o membench/test6.o membench/test7.o netbench.o $(addsuffix .o,$(SUB)) ../libc/lib.o

BUILD = ../../build/
DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

GIT_BRANCH = $(shell git symbolic-ref --short HEAD)

FLAGS = 

INCLUDE_PATH = -I$(realpath ../libc/) -I$(realpath ./)  -I$(realpath ./freebsd/) -I$(realpath ./acpica/include/)
RAPHFLAGS = -ggdb3 -O0 -Wall -Werror=return-type -Wshadow -nostdinc -nostdlib -fno-builtin -mcmodel=large -mno-mmx -D__KERNEL__ -D_KERNEL -Dbootverbose=0 $(FLAGS) $(INCLUDE_PATH) -MMD -MP
RAPHCFLAGS = $(RAPHFLAGS) -std=c99
RAPHCXXFLAGS = $(RAPHFLAGS) -std=c++1y -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-threadsafe-statics
CFLAGS += $(RAPHCFLAGS)
CXXFLAGS += $(RAPHCXXFLAGS)
ASFLAGS += $(RAPHFLAGS)
export CFLAGS
export CXXFLAGS
export ASFLAGS

.PHONY: clean

default:
	@$(foreach rule, $(SUB), $(MAKE) -C $(rule);)
	@$(MAKE) -C ../libc
	@$(MAKE) $(BUILD)$(KERNEL)

-include $(DEPS)

$(BUILD)$(KERNEL): $(OBJS) kernel.ld
	$(LD) -nostdinc -nostdlib -Tkernel.ld $(OBJS) -o $@
	@{ echo -n "commit: "; git log -n 1 --format=%H; echo -n "build time: "; TZ='Asia/Tokyo' date "+%Y%m%d-%H%M%S (JST)"; } > $(BUILD)timestamp
	@echo "build information:"
	@cat $(BUILD)timestamp

clean:
	@$(foreach rule, $(SUB), $(MAKE) -C $(rule) clean;)
	$(MAKE) -C ../libc clean
	-rm -f $(OBJS) $(DEPS) *.s *.ii
