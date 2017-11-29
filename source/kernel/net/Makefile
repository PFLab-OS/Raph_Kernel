OBJS = udp.o 

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean

include $(RAPH_PROJECT_ROOT)/common_rules.mk

default:

build:
	@$(MAKE) ../net.o

-include $(DEPS)

../net.o: $(OBJS)
	ld -r $^ -o $@

clean:
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
