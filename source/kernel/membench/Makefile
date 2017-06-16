OBJS = membench.o test2.o test3.o test4.o test5.o test6.o test7.o

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

CXXFLAGS := $(subst -O0,-O2,$(subst -ggdb3,,$(CXXFLAGS)))

.PHONY: clean

default:
	@$(MAKE) ../membench.o

-include $(DEPS)

../membench.o: $(OBJS)
	ld -r $^ -o $@

clean:
	-rm -f $(OBJS) $(DEPS) *.s *.ii
