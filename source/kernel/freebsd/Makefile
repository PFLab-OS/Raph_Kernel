OBJS = sys/taskqueue.o sys/bus.o sys/rman.o sys/bus_dma.o

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean

default: ../freebsd.o

-include $(DEPS)

../freebsd.o: $(OBJS)
	ld -r $^ -o $@

clean:
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
