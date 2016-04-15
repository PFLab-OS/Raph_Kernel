#CC = g++
SRCDIR += ./hardware ./resources ./dispatcher ./tables ./events ./namespace ./utilities ./executer ./parser 
HEADERDIR += ./include/ ./include/platform/ ./
HEADERS = $(foreach dir, $(HEADERDIR), $(wildcard $(dir)*h))
SRCS += $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.c)) ./osraphinexf.c
OBJS += $(SRCS:.c=.o)
CFLAGS += $(addprefix -I, $(HEADERDIR)) 
CFLAGS += -D__RAPHINE__
ACPICA = acpica

.PHONY: test classtest
default:acpica.o $(OBJS)


test:$(OBJS)
	g++ acpica_test.c $(OBJS) $(CFLAGS) -o test.out
	./test.out

classtest:$(OBJS) acpica.o
	g++ classtest.c $(OBJS) acpica.o $(CFLAGS) -o test.out
	./test.out

.PHONY: clean echo
clean:
	rm  *.o $(OBJS) test.out
echo:
	@echo "SOURCE FILES:"
	@echo $(SRCS)
	@echo "HEADER FILES:"
	@echo $(HEADERS)


