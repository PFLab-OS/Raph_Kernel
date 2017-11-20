OBJS = rl.o 

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean

include $(RAPH_PROJECT_ROOT)/common_rules.mk

default:

build: ../rtl8139.o

-include $(DEPS)

../rtl8139.o: $(OBJS)
	ld -r $^ -o $@

clean:
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
