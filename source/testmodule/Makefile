MODULE01 = testmodule
MODULE02 = forked
#SRCS = testmodule.cc stubstart.S
SRCS01 = testmodule.cc
SRCS02 = forked.cc
CC = musl-gcc

defalut:

build:
	$(MAKE) $(MODULE01)
	$(MAKE) $(MODULE02)
	cp $(MODULE01) ../../build/$(MODULE01).elf
	cp $(MODULE02) ../../build/$(MODULE02).elf

$(MODULE01) : $(SRCS01) Makefile
#	g++ -nostdlib -fPIE -static -o $(MODULE) $(SRCS)
	$(CC) -g0 -mcmodel=large -static -o $(MODULE01) $(SRCS01)

$(MODULE02) : $(SRCS02) Makefile
#	g++ -nostdlib -fPIE -static -o $(MODULE) $(SRCS)
	$(CC) -g0 -mcmodel=large -static -o $(MODULE02) $(SRCS02)

clean:
	-rm $(MODULE01)
	-rm $(MODULE02)
	-rm *.o
