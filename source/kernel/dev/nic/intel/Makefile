SUB = em ix
OBJS = $(addsuffix .o,$(SUB))

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean

default:
	@$(foreach rule, $(SUB), $(MAKE) -C $(rule);)
	@$(MAKE) ../intel.o

-include $(DEPS)

../intel.o: $(OBJS)
	ld -r $^ -o $@

clean:
	@$(foreach rule, $(SUB), $(MAKE) -C $(rule) clean;)
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
