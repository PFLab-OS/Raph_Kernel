SUB = intel
OBJS = $(addsuffix .o,$(SUB))

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean

default:
	@$(foreach rule, $(SUB), $(MAKE) -C $(rule);)
	@$(MAKE) ../nic.o

-include $(DEPS)

../nic.o: $(OBJS)
	ld -r $^ -o $@

clean:
	@$(foreach rule, $(SUB), $(MAKE) -C $(rule) clean;)
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
