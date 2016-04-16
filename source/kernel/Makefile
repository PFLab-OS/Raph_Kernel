KERNEL = kernel.elf
NET_OBJS = net/eth.o net/arp.o net/ip.o net/tcp.o net/udp.o net/socket.o net/netctrl.o
ACPICA = acpica/acpica.a
OBJS = boot/boot.o boot/bootap.o main.o spinlock.o apic.o acpi.o gdt.o idt.o int.o tty.o task.o queue.o functional.o dev.o mem/virtmem.o mem/tmpmem.o mem/paging.o mem/physmem.o $(DEV_OBJS) $(NET_OBJS) $(ACPICA)
TEST_OBJS = test_main.o spinlock.o mem/virtmem.o dev/raw.o dev/netdev.o net/test.o $(NET_OBJS)



# ACPICASRCDIR += ./acpica/hardware ./acpica/resources ./acpica/dispatcher ./acpica/tables ./acpica/events ./acpica/namespace ./acpica/utilities ./acpica/executer ./acpica/parser 
# ACPICAHEADERDIR += ./acpica/include/ ./acpica/include/platform/
# ACPICACHEADERS = $(foreach dir, $(ACPICAHEADERDIR), $(wildcard $(dir)*h)) ./acpica/acraphine.h ./acpica/acraphineex.h
# ACPICACSRCS += $(foreach dir, $(ACPICASRCDIR), $(wildcard $(dir)/*.c)) ./acpica/osraphinexf.c
# ACPICACOBJS += $(ACPICACSRCS:.c=.o)
# ACPICACFLAGS += $(addprefix -I, $(ACPICAHEADERDIR))


BUILD = ../../build/
DEPS= $(filter %.d,$(OBJS:%.o=%.d) $(TEST_OBJS:%.o=%.d))

TEST = ./unit_test

CXXFLAGS += -Wall -Wshadow -nostdinc -nostdlib -fno-exceptions -fno-rtti -fno-use-cxa-atexit -mcmodel=large -D__KERNEL__ -D__RAPHINE__ -std=c++11 -I$(realpath ../lib/) -I$(realpath .) -MMD -MP -Wl,--whole-archive

ASFLAGS += -I$(realpath .)

.PHONY: clean test

default: $(BUILD)$(KERNEL)

$(ACPICA):
	make -C  acpica

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

echo:
	@echo FLAG
	@echo $(ACPICACFLAGS)
	@echo OBJS
	@echo $(ACPICACOBJS)

dev.o:
	make CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)" -C dev obj

clean:
	make -C dev clean
	make -C acpica clean
	-rm -f $(OBJS) $(TEST) $(DEPS) *.s *.ii
