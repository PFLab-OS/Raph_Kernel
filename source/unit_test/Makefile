OBJS = $(subst .cc,.o, $(wildcard *.cc))

DEPS = $(filter %.d, $(subst .o,.d, $(OBJS)))

CFLAGS = $(RAPH_CFLAGS) -pthread -D__TEST__
CXXFLAGS = $(RAPH_CXXFLAGS) -pthread -D__TEST__

.PHONY: test

include $(RAPH_PROJECT_ROOT)/common_rules.mk
-include $(DEPS)

default:

test: ./unit_test
	@./unit_test

./unit_test: $(OBJS)
	@g++ $^ -pthread -lpthread -o $@
	@echo "Test start!"

clean:
	-rm -f unit_test
	-rm -f $(OBJS) $(DEPS)
