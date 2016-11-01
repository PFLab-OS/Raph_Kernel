OBJS = psocket.o pstack.o socket.o eth.o arp.o ip.o udp.o

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean

default: ../net.o

-include $(DEPS)

../net.o: $(OBJS)
	ld -r $^ -o $@

clean:
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
