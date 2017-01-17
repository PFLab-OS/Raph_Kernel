OBJS = fat.o

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean fat.o

default: ../fs.o

-include $(DEPS)

../fs.o: $(OBJS)
	ld -r $^ -o $@

fat.o:
	@$(MAKE) -C fat CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

clean:
	$(MAKE) -C fat clean
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
