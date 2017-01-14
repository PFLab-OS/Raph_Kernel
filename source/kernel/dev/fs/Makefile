OBJS = fat.o

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean fat.o

default: ../fs.o

-include $(DEPS)

../fs.o: $(OBJS)
	ld -r $^ -o $@

fat.o:
	@make -C fat CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)"

clean:
	make -C fat clean
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
