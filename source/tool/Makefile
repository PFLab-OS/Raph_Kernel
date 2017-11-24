.PHONY: clean

defalut:

build:
	$(MAKE) mkfs

mkfs: mkfs.c
	$(CC) $< -o $@

clean:
	-rm -f mkfs
