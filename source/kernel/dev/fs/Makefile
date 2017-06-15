SUB = fat
OBJS = $(addsuffix .o,$(SUB))

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean

default:
	@$(foreach rule, $(SUB), $(MAKE) -C $(rule);)
	@$(MAKE) ../fs.o

-include $(DEPS)

../fs.o: $(OBJS)
	ld -r $^ -o $@

clean:
	@$(foreach rule, $(SUB), $(MAKE) -C $(rule) clean;)
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
