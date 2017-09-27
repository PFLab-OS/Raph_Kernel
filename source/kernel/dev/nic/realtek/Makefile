SUB = rtl8139
OBJS = $(addsuffix .o,$(SUB))

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean

default:

build:
	@$(foreach rule, $(SUB), $(MAKE) -C $(rule) build;)
	@$(MAKE) ../realtek.o

-include $(DEPS)

../realtek.o: $(OBJS)
	ld -r $^ -o $@

clean:
	@$(foreach rule, $(SUB), $(MAKE) -C $(rule) clean;)
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
