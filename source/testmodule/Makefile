MODULE = testmodule
#SRCS = testmodule.cc stubstart.S
SRCS = testmodule.cc

defalut:
	make $(MODULE)
	cp $(MODULE) ../../build/$(MODULE).elf

$(MODULE) : $(SRCS) Makefile
	musl-gcc -mcmodel=large -pie -fPIE -static -o $(MODULE) $(SRCS)
#	g++ -fpic -nostdlib -static -o $(MODULE) $(SRCS)

clean:
	-rm $(MODULE)
	-rm *.o
