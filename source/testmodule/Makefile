MODULE = testmodule
#SRCS = testmodule.cc stubstart.S
#SRCS = testmodule.cc
SRCS = udpecho.cc
CC = musl-gcc

defalut: $(MODULE)

build: ../../build/$(MODULE).elf

../../build/$(MODULE).elf: $(MODULE)
	cp $^ $@

$(MODULE) : $(SRCS) Makefile
#	g++ -nostdlib -fPIE -static -o $(MODULE) $(SRCS)
	$(CC) -g0 -mcmodel=large -static -o $(MODULE) $(SRCS)

clean:
	-rm -f $(MODULE)
	-rm -f *.o
