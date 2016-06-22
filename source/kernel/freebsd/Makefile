OBJS = sys/bus.o sys/mutex.o sys/bus_dma.o sys/rman.o sys/callout.o sys/taskqueue.o sys/kernel.o dev/pci/pcivar.o

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean

default: ../freebsd.o

-include $(DEPS)

../freebsd.o: $(OBJS)
	ld -r $^ -o $@

clean:
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
