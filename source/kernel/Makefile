KERNEL = kernel.elf

NET_OBJS = net/eth.o net/arp.o net/ip.o net/tcp.o net/udp.o net/socket.o net/netctrl.o
FREEBSD_OBJS = freebsd/sys/taskqueue.o
OBJS = boot/boot.o boot/bootap.o main.o spinlock.o apic.o raph_acpi.o gdt.o idt.o int.o tty.o task.o queue.o functional.o dev.o acpica.o mem/virtmem.o mem/tmpmem.o mem/paging.o mem/physmem.o $(FREEBSD_OBJS) $(NET_OBJS)
TEST_OBJS = test_main.o spinlock.o mem/virtmem.o dev/raw.o dev/netdev.o net/test.o $(NET_OBJS)

BUILD = ../../build/
DEPS= $(filter %.d, $(subst .o,.d, $(OBJS) $(TEST_OBJS)))

TEST = ./unit_test

RAPHFLAGS = -Wall -Wshadow -nostdinc -nostdlib -fno-builtin -mcmodel=large -D__KERNEL__ -I$(realpath ../lib/) -I$(realpath .) -I$(realpath ./acpica/include) -MMD -MP
RAPHCFLAGS = $(RAPHFLAGS) -std=c99
RAPHCXXFLAGS = $(RAPHFLAGS) -std=c++11 -fno-exceptions -fno-rtti -fno-use-cxa-atexit
CFLAGS += $(RAPHCFLAGS)
CXXFLAGS += $(RAPHCXXFLAGS)

ASFLAGS += -I$(realpath .)

.PHONY: clean test dev.o acpica.o

default: $(BUILD)$(KERNEL)

-include $(DEPS)

test:
	make $(TEST) CXX=g++ LD=ld "CXXFLAGS= -O3 -g -Wall -Wshadow -D__UNIT_TEST__ -std=c++11 -I."
	rm $(TEST_OBJS)

testrun:
	make clean
	make test
	sudo $(TEST)

$(TEST): $(TEST_OBJS)
	g++ -pedantic -pthread -static -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -o $@ $^

$(BUILD)$(KERNEL): $(OBJS) kernel.ld
	$(LD) -nostdinc -nostdlib -Tkernel.ld $(OBJS) -o $@

dev.o:
	make -C dev CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

acpica.o:
	make -C acpica CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)"

clean:
	make -C dev clean
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
