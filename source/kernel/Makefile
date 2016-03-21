KERNEL = kernel.elf
DEV_OBJ = dev/pci.o dev/e1000/lem.o dev/e1000/em.o dev/e1000/e1000_api.o dev/e1000/e1000_nvm.o dev/e1000/e1000_mac.o dev/e1000/e1000_phy.o dev/e1000/e1000_manage.o dev/e1000/e1000_80003es2lan.o dev/e1000/e1000_82540.o dev/e1000/e1000_82541.o dev/e1000/e1000_82542.o dev/e1000/e1000_82543.o dev/e1000/e1000_82571.o dev/e1000/e1000_ich8lan.o dev/e1000/e1000_osdep.o dev/e1000/e1000_raph.o dev/e1000/if_lem.o dev/e1000/if_em.o dev/oe1000/e1000.o dev/oe1000/i8254.o dev/oe1000/i8257.o dev/oe1000/ich8.o
OBJ = boot/boot.o boot/bootap.o main.o spinlock.o apic.o acpi.o idt.o int.o mem/virtmem.o mem/paging.o mem/physmem.o $(DEV_OBJ)
TEST_OBJ = test_main.o spinlock.o list.o mem/virtmem.o mem/paging.o mem/physmem.o
BUILD = ../../build/
DEPS= $(OBJ:%.o=%.d) $(TEST_OBJ:%.o=%.d)

TEST = ./unit_test

CXXFLAGS += -Wall -Wshadow -nostdinc -nostdlib -fno-exceptions -fno-rtti -mcmodel=large -D__KERNEL__ -std=c++11 -I../lib/ -I. -MMD -MP

.PHONY: clean test

default: $(BUILD)$(KERNEL)

-include $(DEPS)

test: 
	make $(TEST) CXX=g++ LD=ld "CXXFLAGS= -O3 -g -Wall -Wshadow -D__UNIT_TEST__ -std=c++11"
	rm $(TEST_OBJ)

testrun:
	make clean
	make test
	$(TEST)

$(TEST): $(TEST_OBJ)
	g++ -pedantic -pthread -static -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -o $@ $^

$(BUILD)$(KERNEL): $(OBJ) kernel.ld
	$(LD) -nostdinc -nostdlib -Tkernel.ld $(OBJ) -o $@

clean:
	-rm -f $(OBJ) $(TEST) $(DEPS)
