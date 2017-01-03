MODULE = testmodule
#SRCS = testmodule.cc stubstart.S
SRCS = testmodule.cc
CC = /usr/local/musl/bin/musl-gcc

defalut:
	make $(MODULE)
	cp $(MODULE) ../../build/$(MODULE).elf

$(MODULE) : $(SRCS) Makefile
#	g++ -nostdlib -fPIE -static -o $(MODULE) $(SRCS)
	$(CC) -mcmodel=large -pie -fPIE -static -o $(MODULE) $(SRCS)

clean:
	-rm $(MODULE)
	-rm *.o
