##このメイクファイルで .a を作ればよい

#CC = g++
SRCDIR += ./hardware ./resources ./dispatcher ./tables ./events ./namespace ./utilities ./executer ./parser 
HEADERDIR += ./include/ ./include/platform/ ./ $(realpath ../)
HEADERS += $(foreach dir, $(HEADERDIR), $(wildcard $(dir)*h)) ./acpica.h ./osraphinexf.h
#HEADERS += $(foreach dir, $(HEADERDIR), $(wildcard $(dir)*h)) 
#SRCS += $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.c)) ./osraphinexf.c  utilities/utclib.c 
SRCS += $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.c))   utilities/utclib.c  ./osraphinexf.c
OBJS += $(SRCS:.c=.o) 
CFLAGS += $(addprefix -I, $(HEADERDIR)) 
CFLAGS += -D__RAPHINE__ -fPIC -MMD
ACPICA += libacpica.a

.PHONY: test classtest
default:acpica.o $(OBJS)
	ar rcs $(ACPICA) acpica.o $(OBJS) $(HEADERS)
	rm *.o

classtest:$(OBJS) acpica.o
	g++ classtest.c $(OBJS) acpica.o $(CFLAGS) -o test.out
	./test.out


test:$(OBJS)
	g++ acpica_test.c $(OBJS) $(CFLAGS) -o test.out
	./test.out


.PHONY: clean echo
clean:
	-rm  *.o $(OBJS) test.out *.a


dry:acpica.o $(OBJS)
	gcc -E -DD acpica.o $(OBJS) $(HEADERS)
	rm *.o

echo:
	@echo "SOURCE FILES:"
	@echo $(SRCS)
	@echo "HEADER FILES:"
	@echo $(HEADERS)
	@echo $(HEADERDIR)
