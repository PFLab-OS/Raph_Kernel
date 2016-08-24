KERNEL = kernel.elf

NET_OBJS = net/eth.o net/arp.o net/ip.o net/tcp.o net/udp.o net/socket.o net/netctrl.o net/ptcl.o
OBJS = boot/boot.o boot/bootap.o main.o apic.o raph_acpi.o gdt.o idt.o int.o freebsd.o dev.o acpica.o mem/kvirtmem.o shell.o mem/paging.o mem/physmem.o mem/dlmalloc.o $(NET_OBJS) ../Raph_Library/lib.o
TEST_OBJS = mem/kvirtmem.o dev/raw.o dev/netdev.o $(NET_OBJS) ../Raph_Library/lib.o

BUILD = ../../build/
DEPS= $(filter %.d, $(subst .o,.d, $(OBJS) $(TEST_OBJS)))

INCLUDE_PATH = -I$(realpath ../Raph_Library/libc/) -I$(realpath ../Raph_Library/rlib/) -I$(realpath ./)  -I$(realpath ./freebsd/) -I$(realpath ./acpica/include/)
RAPHFLAGS = -g -Wall -Wshadow -nostdinc -nostdlib -fno-builtin -mcmodel=large -D__KERNEL__ -D_KERNEL -Dbootverbose=0 $(INCLUDE_PATH) -MMD -MP
RAPHCFLAGS = $(RAPHFLAGS) -std=c99
RAPHCXXFLAGS = $(RAPHFLAGS) -std=c++1y -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-threadsafe-statics
CFLAGS += $(RAPHCFLAGS)
CXXFLAGS += $(RAPHCXXFLAGS)

ASFLAGS += $(INCLUDE_PATH)

.PHONY: clean dev.o freebsd.o acpica.o ../Raph_Library/lib.o

default: $(BUILD)$(KERNEL)

-include $(DEPS)

$(TEST): $(TEST_OBJS)
	g++ -pedantic -pthread -static -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -o $@ $^

$(BUILD)$(KERNEL): $(OBJS) kernel.ld
	$(LD) -nostdinc -nostdlib -Tkernel.ld $(OBJS) -o $@

dev.o:
	make -C dev CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

freebsd.o:
	make -C freebsd CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

acpica.o:
	make -C acpica CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)"

../Raph_Library/lib.o:
	make -C ../Raph_Library CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)"

clean:
	make -C dev clean
	make -C freebsd clean
	make -C acpica clean
	make -C ../Raph_Library clean
	-rm -f $(OBJS) $(TEST_OBJS) $(TEST) $(DEPS) *.s *.ii
