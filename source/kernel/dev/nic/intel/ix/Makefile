OBJS = if_ix.o ix_txrx.o ixgbe_82598.o ixgbe_82599.o ixgbe_api.o ixgbe_common.o ixgbe_dcb.o ixgbe_dcb_82598.o ixgbe_dcb_82599.o ixgbe_mbx.o ixgbe_osdep.o ixgbe_phy.o ixgbe_vf.o ixgbe_x540.o ixgbe_x550.o

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean

include $(RAPH_PROJECT_ROOT)/common_rules.mk

default:

build: ../ix.o

-include $(DEPS)

../ix.o: $(OBJS)
	ld -r $^ -o $@

clean:
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
