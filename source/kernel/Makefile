KERNEL = kernel.elf
DEV_OBJ = dev/pci.o dev/e1000.o
NET_OBJ = net/segment.o net/eth.o net/ip.o net/tcp.o
OBJ = boot/boot.o boot/bootap.o main.o spinlock.o apic.o acpi.o idt.o int.o mem/virtmem.o mem/paging.o mem/physmem.o $(DEV_OBJ) $(NET_OBJ)
TEST_OBJ = test_main.o spinlock.o list.o mem/virtmem.o mem/paging.o mem/physmem.o $(NET_OBJ)
BUILD = ../../build/
DEPS= $(OBJ:%.o=%.d) $(TEST_OBJ:%.o=%.d)

TEST = ./unit_test

CXXFLAGS += -Wall -Wshadow -nostdinc -nostdlib -fno-exceptions -fno-rtti -mcmodel=large -D__KERNEL__ -std=c++11 -I../lib/ -MMD -MP

.PHONY: clean test

default: $(BUILD)$(KERNEL)

-include $(DEPS)

test: 
	make $(TEST) CXX=g++ LD=ld "CXXFLAGS= -O3 -g -Wall -Wshadow -D__UNIT_TEST__ -std=c++11"
	rm $(TEST_OBJ)

testrun:
	make clean
	make test
	sudo $(TEST)

$(TEST): $(TEST_OBJ)
	g++ -pedantic -pthread -static -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -o $@ $^

$(BUILD)$(KERNEL): $(OBJ) kernel.ld
	$(LD) -nostdinc -nostdlib -Tkernel.ld $(OBJ) -o $@

clean:
	-rm -f $(OBJ) $(TEST) $(DEPS)
