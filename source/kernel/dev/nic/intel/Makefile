OBJS = em.o ix.o

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean em.o ix.o

default: ../intel.o

-include $(DEPS)

../intel.o: $(OBJS)
	ld -r $^ -o $@

em.o:
	make -C em CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

ix.o:
	make -C ix CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

clean:
	make -C em clean
	make -C ix clean
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
