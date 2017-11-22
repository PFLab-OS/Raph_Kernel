
%.o : %.cc Makefile
	@echo "CXX $*.cc"
	@$(CXX) $(CXXFLAGS) -c -o $@ $*.cc

%.o : %.S Makefile
	@echo "CC $*.S"
	@$(CC) $(CFLAGS) -c -o $@ $*.S

%.o : %.c Makefile
	@echo "CC $*.c"
	@$(CC) $(CFLAGS) -c -o $@ $*.c
