MODULE01 = testmodule
MODULE02 = forked
MODULE03 = init
#SRCS = testmodule.cc stubstart.S
SRCS01 = testmodule.cc
SRCS02 = forked.cc
SRCS03 = init.cc
CC = musl-gcc

defalut:

build:
	$(MAKE) $(MODULE01)
	$(MAKE) $(MODULE02)
	$(MAKE) $(MODULE03)
	cp $(MODULE01) ../../build/$(MODULE01).elf
	cp $(MODULE02) ../../build/$(MODULE02).elf
	cp $(MODULE03) ../../build/$(MODULE03).elf

$(MODULE01) : $(SRCS01) Makefile
#	g++ -nostdlib -fPIE -static -o $(MODULE) $(SRCS)
	$(CC) -g0 -mcmodel=large -static -o $(MODULE01) $(SRCS01)

$(MODULE02) : $(SRCS02) Makefile
#	g++ -nostdlib -fPIE -static -o $(MODULE) $(SRCS)
	$(CC) -g0 -mcmodel=large -static -o $(MODULE02) $(SRCS02)

$(MODULE03) : $(SRCS03) Makefile
#	g++ -nostdlib -fPIE -static -o $(MODULE) $(SRCS)
	$(CC) -g0 -mcmodel=large -static -o $(MODULE03) $(SRCS03)

clean:
	-rm $(MODULE01)
	-rm $(MODULE02)
	-rm $(MODULE03)
	-rm *.o
