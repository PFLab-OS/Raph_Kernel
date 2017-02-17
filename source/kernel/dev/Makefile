OBJS = pci.o usb.o nic.o netdev.o disk.o fs.o 8042.o keyboard.o pciid.o pciid_table.o

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
	$(MAKE) -C usb CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

nic.o:
	@$(MAKE) -C nic CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

disk.o:
	@$(MAKE) -C disk CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

fs.o:
	@$(MAKE) -C fs CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

clean:
	$(MAKE) -C nic clean
	$(MAKE) -C disk clean
	$(MAKE) -C fs clean
	$(MAKE) -C usb clean
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
	-rm -f pciid_table.cc pci.ids.formatted
