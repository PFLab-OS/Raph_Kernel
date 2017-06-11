OBJS = rtl8139.o

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean 

default: ../realtek.o

-include $(DEPS)

../realtek.o: $(OBJS)
	ld -r $^ -o $@

rtl8139.o:
	@$(MAKE) -C rtl8139 

clean:
	$(MAKE) -C realtek clean
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
