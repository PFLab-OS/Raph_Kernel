OBJS = intel.o

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean intel.o

default: ../nic.o

-include $(DEPS)

../nic.o: $(OBJS)
	ld -r $^ -o $@

intel.o:
	make -C intel CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

clean:
	make -C intel clean
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
