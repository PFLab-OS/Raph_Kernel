OBJS = usb11.o uhci.o ehci.o

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean

default: ../usb.o

-include $(DEPS)

../usb.o: $(OBJS)
	ld -r $^ -o $@

clean:
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
