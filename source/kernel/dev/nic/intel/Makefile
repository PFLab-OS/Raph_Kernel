OBJS = em.o

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean em.o

default: ../intel.o

-include $(DEPS)

../intel.o: $(OBJS)
	ld -r $^ -o $@

em.o:
	make -C em CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

clean:
	make -C em clean
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
