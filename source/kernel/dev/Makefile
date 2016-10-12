OBJS = pci.o nic.o netdev.o keyboard.o pciid.o

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean nic.o

default: ../dev.o

-include $(DEPS)

../dev.o: $(OBJS)
	ld -r $^ -o $@

pciid.o: pciid.cc
	g++ $(CXXFLAGS) -c $^

pciid.cc: pci.ids.formatted pciid.py
	python pciid.py > pciid.cc

pci.ids.formatted : pci.ids
	sed 's/"/\\\"/g' pci.ids > pci.ids.formatted

nic.o:
	make -C nic CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

clean:
	make -C nic clean
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
