OBJS = pci.o nic.o netdev.o keyboard.o pciid.o pciid_table.o

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean nic.o

default: ../dev.o

-include $(DEPS)

../dev.o: $(OBJS)
	ld -r $^ -o $@

pciid_table.o: pciid_table.cc
	g++ $(CXXFLAGS) -c $^

pciid_table.cc: pci.ids.formatted pciid.py
	python pciid.py > pciid_table.cc

pci.ids.formatted : pci.ids
	sed 's/\("\|\?\|\\\)/\\\1/g' pci.ids > pci.ids.formatted

nic.o:
	make -C nic CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

clean:
	make -C nic clean
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
	rm pciid_table.cc pci.ids.formatted
