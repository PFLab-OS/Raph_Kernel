NIC_ARCHIVE = ../nic.o
NIC_OBJS = intel.o
DEPS= $(filter %.d,$(NIC_OBJS:%.o=%.d))

obj: $(NIC_ARCHIVE)

$(NIC_ARCHIVE): $(NIC_OBJS)
	ld -r $^ -o $@

intel.o:
	make CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)" -C intel obj

clean:
	make -C intel clean
	-rm -f $(NIC_OBJS) $(DEPS) *.s *.ii

