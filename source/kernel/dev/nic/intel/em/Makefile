OBJS = e1000_api.o e1000_nvm.o e1000_mac.o e1000_phy.o e1000_manage.o e1000_80003es2lan.o e1000_82540.o e1000_82541.o e1000_82542.o e1000_82543.o e1000_82571.o e1000_ich8lan.o e1000_osdep.o e1000_raph.o if_lem.o if_em.o

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean

default: ../em.o

-include $(DEPS)

../em.o: $(OBJS)
	ld -r $^ -o $@

clean:
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
