EM_ARCHIVE = ../em.o
EM_OBJS = lem.o em.o e1000_api.o e1000_nvm.o e1000_mac.o e1000_phy.o e1000_manage.o e1000_80003es2lan.o e1000_82540.o e1000_82541.o e1000_82542.o e1000_82543.o e1000_82571.o e1000_ich8lan.o e1000_osdep.o e1000_raph.o if_lem.o if_em.o
DEPS= $(EM_OBJS:%.o=%.d)

obj: $(EM_ARCHIVE)

$(EM_ARCHIVE): $(EM_OBJS)
	ld -r $^ -o $@

clean:
	-rm -f $(EM_OBJS) $(DEPS) *.s *.ii
