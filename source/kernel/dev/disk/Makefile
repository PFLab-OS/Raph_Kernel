OBJS = ahci.o

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean ahci.o

default: ../disk.o

-include $(DEPS)

../disk.o: $(OBJS)
	ld -r $^ -o $@

ahci.o:
	make -C ahci CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

clean:
	make -C ahci clean
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
