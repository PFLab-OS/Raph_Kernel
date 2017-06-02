OBJS = em.o ix.o

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean

default: ../intel.o

-include $(DEPS)

../intel.o: $(OBJS)
	ld -r $^ -o $@

em.o:
	@$(MAKE) -C em CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

ix.o:
	@$(MAKE) -C ix CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

clean:
	$(MAKE) -C em clean
	$(MAKE) -C ix clean
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
