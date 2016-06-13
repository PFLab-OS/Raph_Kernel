OBJS = pci.o nic.o disk.o netdev.o eth.o keyboard.o

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean nic.o disk.o

default: ../dev.o

-include $(DEPS)

../dev.o: $(OBJS)
	ld -r $^ -o $@

nic.o:
	make -C nic CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

disk.o:
	make -C disk CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

clean:
	make -C nic clean
	make -C disk clean
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
