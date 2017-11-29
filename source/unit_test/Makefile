OBJS = $(subst .cc,.o, $(wildcard *.cc))

DEPS = $(filter %.d, $(subst .o,.d, $(OBJS)))

CFLAGS = $(RAPH_CFLAGS)
CXXFLAGS = $(RAPH_CXXFLAGS)

.PHONY: test

include $(RAPH_PROJECT_ROOT)/common_rules.mk
-include $(DEPS)

default:

test: ./unit_test
	@./unit_test

./unit_test: $(OBJS)
	@g++ $^ -o $@
	@echo "Test start!"

clean:
	-rm -f unit_test
	-rm -f $(OBJS) $(DEPS)
