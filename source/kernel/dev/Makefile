SUB = usb nic disk fs
OBJS = pci.o netdev.o 8042.o keyboard.o pciid.o pciid_table.o hpet.o framebuffer.o $(addsuffix .o,$(SUB))

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean

default:
	@$(foreach rule, $(SUB), $(MAKE) -C $(rule);)
	@$(MAKE) ../dev.o

-include $(DEPS)

../dev.o: $(OBJS)
	ld -r $^ -o $@

pciid_table.o: pciid_table.cc
	g++ $(CXXFLAGS) -c $^

pciid_table.cc: pci.ids.formatted pciid.py
	python pciid.py > pciid_table.cc

pci.ids.formatted : pci.ids
	sed 's/\("\|\?\|\\\)/\\\1/g' pci.ids > pci.ids.formatted

clean:
	@$(foreach rule, $(SUB), $(MAKE) -C $(rule) clean;)
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
	-rm -f pciid_table.cc pci.ids.formatted
