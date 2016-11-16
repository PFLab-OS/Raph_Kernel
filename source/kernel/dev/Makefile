OBJS = pci.o usb.o nic.o netdev.o disk.o fs.o keyboard.o pciid.o pciid_table.o

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean usb.o nic.o disk.o fs.o

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

usb.o:
	make -C usb CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

nic.o:
	make -C nic CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

disk.o:
	make -C disk CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

fs.o:
	make -C fs CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

clean:
	make -C usb clean
	make -C nic clean
	make -C disk clean
	make -C fs clean
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
	rm pciid_table.cc pci.ids.formatted
