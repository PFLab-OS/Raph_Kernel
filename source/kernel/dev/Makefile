OBJS = pci.o nic.o netdev.o eth.o keyboard.o

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean nic.o

default: ../dev.o

-include $(DEPS)

../dev.o: $(OBJS)
	ld -r $^ -o $@

nic.o:
	make -C nic CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

clean:
	make -C nic clean
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
