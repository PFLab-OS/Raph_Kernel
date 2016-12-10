MODULE = testmodule
SRCS = testmodule.cc

defalut:
	make $(MODULE)
	cp $(MODULE) ../../build/$(MODULE).elf

$(MODULE) : $(SRCS) Makefile
	g++ -fpic -static -o $(MODULE) $(SRCS)

clean:
	-rm $(MODULE)
	-rm *.o
