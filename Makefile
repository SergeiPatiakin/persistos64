# Default target.
.PHONY: all
all: kernel

.PHONY: userspace
userspace:
	$(MAKE) -C userspace

kernel/build/initrd.bin: userspace
	mkdir -p "$$(dirname $@)"
	cp -v userspace/build/initrd.bin kernel/build/initrd.bin

.PHONY: kernel
kernel: kernel/build/initrd.bin
	$(MAKE) -C kernel

.PHONY: clean
clean:
	$(MAKE) -C userspace clean
	$(MAKE) -C kernel clean
