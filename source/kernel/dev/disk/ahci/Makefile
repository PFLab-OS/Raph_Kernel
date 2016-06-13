OBJS = ahci.o ahci_generic.o ahci_pci.o ahciem.o

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean

default: ../ahci.o

-include $(DEPS)

../ahci.o: $(OBJS)
	ld -r $^ -o $@

clean:
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
