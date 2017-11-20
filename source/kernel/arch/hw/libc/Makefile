OBJS = libc.o libc_asm.o

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean

include $(RAPH_PROJECT_ROOT)/common_rules.mk

default: ./lib.o

-include $(DEPS)

./lib.o: $(OBJS)
	ld -r $^ -o $@

clean:
	-rm -f ./lib.o $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
