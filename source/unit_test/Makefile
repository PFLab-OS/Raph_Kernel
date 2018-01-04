SRCS = $(filter-out main.cc, $(wildcard *.cc))
OBJS = $(subst .cc,.o, $(SRCS))
BINS = $(subst .cc,.bin, $(SRCS))
DEPS = $(filter %.d, $(subst .o,.d, $(OBJS)))

CFLAGS = $(RAPH_CFLAGS) -pthread -D__TEST__
CXXFLAGS = $(RAPH_CXXFLAGS) -pthread -D__TEST__

.PHONY: test

include $(RAPH_PROJECT_ROOT)/common_rules.mk
-include $(DEPS)

default:

test: $(BINS)
	@echo "Test start!"
	@$(foreach app, $(BINS), ./$(app) && ):

%.bin: %.o main.o
	g++ $^ -pthread -lpthread -o $@

clean:
	-rm -f unit_test
	-rm -f $(OBJS) $(BINS) $(DEPS)
