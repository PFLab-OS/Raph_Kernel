KERNEL = kernel.elf

EM_OBJS = $(addprefix dev/nic/intel/em/, e1000_api.o e1000_nvm.o e1000_mac.o e1000_phy.o e1000_manage.o e1000_80003es2lan.o e1000_82540.o e1000_82541.o e1000_82542.o e1000_82543.o e1000_82571.o e1000_ich8lan.o e1000_osdep.o e1000_raph.o if_lem.o if_em.o)
INTEL_OBJS = $(addprefix dev/nic/intel/, em.o)
NIC_OBJS = $(addprefix dev/nic/, intel.o)
DEV_OBJS = $(addprefix dev/, pci.o netdev.o keyboard.o nic.o)

NET_OBJS = net/eth.o net/arp.o net/ip.o net/tcp.o net/udp.o net/socket.o net/netctrl.o
FREEBSD_OBJS = freebsd/sys/taskqueue.o
OBJS = boot/boot.o boot/bootap.o main.o spinlock.o apic.o acpi.o gdt.o idt.o int.o tty.o task.o queue.o functional.o dev.o mem/virtmem.o mem/tmpmem.o mem/paging.o mem/physmem.o $(FREEBSD_OBJS) $(NET_OBJS)
TEST_OBJS = test_main.o spinlock.o mem/virtmem.o dev/raw.o dev/netdev.o net/test.o $(NET_OBJS)

BUILD = ../../build/
DEPS= $(filter %.d, $(subst .o,.d, $(OBJS) $(TEST_OBJS) $(NIC_OBJS) $(INTEL_OBJS) $(EM_OBJS)))

TEST = ./unit_test

CXXFLAGS += -Wall -Wshadow -nostdinc -nostdlib -fno-exceptions -fno-rtti -fno-use-cxa-atexit -mcmodel=large -D__KERNEL__ -std=c++11 -I$(realpath ../lib/) -I$(realpath .) -MMD -MP

ASFLAGS += -I$(realpath .)

.PHONY: clean test

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

dev.o: $(DEV_OBJS)
	ld -r $^ -o $@

dev/nic.o: $(NIC_OBJS)
	ld -r $^ -o $@

dev/nic/intel.o: $(INTEL_OBJS)
	ld -r $^ -o $@

dev/nic/intel/em.o: $(EM_OBJS)
	ld -r $^ -o $@

clean:
	-rm -f $(OBJS) $(TEST_OBJS) $(NIC_OBJS) $(INTEL_OBJS) $(EM_OBJS) $(TEST) $(DEPS) *.s *.ii
