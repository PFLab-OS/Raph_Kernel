SUB = ahci
OBJS = $(addsuffix .o,$(SUB))

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean

default:
	@$(foreach rule, $(SUB), $(MAKE) -C $(rule);)
	@$(MAKE) ../disk.o

-include $(DEPS)

../disk.o: $(OBJS)
	ld -r $^ -o $@

clean:
	@$(foreach rule, $(SUB), $(MAKE) -C $(rule) clean;)
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
