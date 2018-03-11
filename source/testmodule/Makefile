MODULE = testmodule
#SRCS = testmodule.cc stubstart.S
SRCS = testmodule.cc
CC = musl-gcc

defalut:

build: ../../build/$(MODULE).elf

../../build/$(MODULE).elf: $(MODULE)
	cp $^ $@

$(MODULE) : $(SRCS) Makefile
#	g++ -nostdlib -fPIE -static -o $(MODULE) $(SRCS)
	$(CC) -g0 -mcmodel=large -fno-stack-protector -static -o $(MODULE) $(SRCS)

clean:
	-rm -f $(MODULE)
	-rm -f *.o
