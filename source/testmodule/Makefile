MODULE = testmodule
#SRCS = testmodule.cc stubstart.S
SRCS = testmodule.cc
CC = musl-gcc

defalut:

build:
	$(MAKE) $(MODULE)
	cp $(MODULE) ../../build/$(MODULE).elf

$(MODULE) : $(SRCS) Makefile
#	g++ -nostdlib -fPIE -static -o $(MODULE) $(SRCS)
	$(CC) -g0 -mcmodel=large -static -o $(MODULE) $(SRCS)

clean:
	-rm -f $(MODULE)
	-rm -f *.o
