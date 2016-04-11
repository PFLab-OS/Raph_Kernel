DEV_ARCHIVE = ../dev.o
DEV_OBJS = pci.o netdev.o keyboard.o nic.o
DEPS= $(filter %.d,$(DEV_OBJS:%.o=%.d))

obj: $(DEV_ARCHIVE)

$(DEV_ARCHIVE): $(DEV_OBJS)
	ld -r $^ -o $@

nic.o:
	make CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)" -C nic obj

clean:
	make -C nic clean
	-rm -f $(DEV_OBJS) $(DEPS) *.s *.ii

