OBJS = intel.o realtek.o

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean

default:

build:
	@$(foreach rule, $(SUB), $(MAKE) -C $(rule) build;)
	@$(MAKE) ../nic.o

-include $(DEPS)

../nic.o: $(OBJS)
	ld -r $^ -o $@

intel.o:
	@$(MAKE) -C intel 

realtek.o:
	@$(MAKE) -C realtek 

clean:
	$(MAKE) -C intel clean
	$(MAKE) -C realtek clean
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
