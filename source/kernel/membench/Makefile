OBJS = membench.o test8.o test9.o test10.o

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

CXXFLAGS := $(subst -O0,-O2,$(subst -ggdb3,,$(CXXFLAGS)))

.PHONY: clean

include $(RAPH_PROJECT_ROOT)/common_rules.mk

default:

build:
	@$(MAKE) ../membench.o

-include $(DEPS)

../membench.o: $(OBJS)
	ld -r $^ -o $@

clean:
	-rm -f $(OBJS) $(DEPS) *.s *.ii
