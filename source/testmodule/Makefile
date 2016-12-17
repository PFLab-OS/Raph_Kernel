MODULE = testmodule
SRCS = testmodule.cc stubstart.S

defalut:
	make $(MODULE)
	cp $(MODULE) ../../build/$(MODULE).elf

$(MODULE) : $(SRCS) Makefile
	g++ -nostdlib -fpic -static -o $(MODULE) $(SRCS)

clean:
	-rm $(MODULE)
	-rm *.o
