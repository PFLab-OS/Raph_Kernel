INTEL_ARCHIVE = ../intel.o
INTEL_OBJS = em.o
DEPS= $(filter %.d,$(INTEL_OBJS:%.o=%.d))

obj: $(INTEL_ARCHIVE)

$(INTEL_ARCHIVE): $(INTEL_OBJS)
	ld -r $^ -o $@

em.o:
	make CXXFLAGS="$(CXXFLAGS)" ASFLAGS="$(ASFLAGS)" -C em obj

clean:
	make -C em clean
	-rm -f $(INTEL_OBJS) $(DEPS) *.s *.ii

