SRCDIR = ./hardware ./resources ./dispatcher ./tables ./events ./namespace ./utilities ./executer ./parser
HEADERDIR = ./include/ ./include/platform/ ./
HEADERS = $(foreach dir, $(HEADERDIR), $(wildcard $(dir)*h))
SRCS = $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.c))
OBJS = $(SRCS:.c=.o) osraphinexf.o acpica_main.o
CFLAGS += $(addprefix -I, $(HEADERDIR)) 
CFLAGS += -D__RAPHINE__
ACPICA = acpica


$(ACPICA): $(OBJS)
	gcc -o $(ACPICA).o $(OBJS) 

.PHONY: clean
clean:
	rm $(ACPICA).o makelog.txt

.PHONY: echo
echo:
	@echo "SOURCE FILES:"
	@echo $(SRCS)
	@echo "HEADER FILES:"
	@echo $(HEADERS)
