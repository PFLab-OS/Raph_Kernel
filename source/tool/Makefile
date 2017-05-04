.PHONY: clean

defalut:
	$(MAKE) mkfs

mkfs: mkfs.c
	$(CC) $< -o $@

clean:
	-rm mkfs
