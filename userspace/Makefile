# Nuke built-in rules and variables.
MAKEFLAGS += -rR
.SUFFIXES:

# Convenience macro to reliably declare user overridable variables.
override USER_VARIABLE = $(if $(filter $(origin $(1)),default undefined),$(eval override $(1) := $(2)))

# User controllable C compiler command (must be clang)
$(call USER_VARIABLE,CC,cc)

# User controllable C flags.
$(call USER_VARIABLE,CFLAGS,-g -O0 -pipe)

# User controllable C preprocessor flags. We set none by default.
$(call USER_VARIABLE,CPPFLAGS,)

# User controllable assembler command (must be clang)
$(call USER_VARIABLE,AS,cc)

# User controllable linker command.
$(call USER_VARIABLE,LD,ld)

# User controllable linker flags. We set none by default.
$(call USER_VARIABLE,LDFLAGS,)


override CFLAGS += \
    -target x86_64-unknown-none \
    -Wall \
    -Wextra \
    -std=gnu11 \
    -ffreestanding \
    -fno-stack-protector \
    -fno-stack-check \
    -fno-lto \
    -fno-PIC \
    -ffunction-sections \
    -fdata-sections \
    -m64 \
    -march=x86-64 \
    -mno-80387 \
    -mno-mmx \
    -mno-sse \
    -mno-sse2 \
    -mno-red-zone \
    -mcmodel=small

override CPPFLAGS := \
    -I src \
    $(CPPFLAGS) \
    -MMD \
    -MP

override ASFLAGS += \
    -target x86_64-unknown-none \
    -Wall \
    -Wextra \
    -fno-lto \
    -fno-PIC \
    -m64 \
    -march=x86-64 \
    -mno-80387 \
    -mno-mmx \
    -mno-sse \
    -mno-sse2 \

override LDFLAGS += \
    -m elf_x86_64 \
    -nostdlib \
    -static \
    -z max-page-size=0x1000 \
    -gc-sections \
    -T linker.ld

# Default target.
.PHONY: all
all: build/initrd.bin

EXECUTABLE_FILES = \
    init \
	shell \
	loop \
	echo \
	fault \
	clear \
	touch \
	ls \
	mkdir \
	increment \
	hd \
	truncate \
	cat \
	test.sh \
	diff \
	ps \
	kill \
	sleep \
	mount
EXECUTABLE_TARGETS = $(addprefix build/, $(EXECUTABLE_FILES))
EXECUTABLE_TARGETS_RELPATHS = $(addprefix bin/, $(EXECUTABLE_FILES))

# Tarball
build/initrd.bin: Makefile $(EXECUTABLE_TARGETS)
	rm -rf build/bin
	mkdir -p build/bin
	cp $(EXECUTABLE_TARGETS) build/bin/
	cd build && tar -cvf initrd.bin bin

LIBC_OBJECT_FILES = build/persistos.c.o build/persistos.s.o build/cstd.c.o

# Link rules
build/init: Makefile linker.ld build/init.c.o $(LIBC_OBJECT_FILES)
	mkdir -p "$$(dirname $@)"
	$(LD) build/init.c.o $(LIBC_OBJECT_FILES) $(LDFLAGS) -o $@

build/shell: Makefile linker.ld build/shell.c.o $(LIBC_OBJECT_FILES)
	mkdir -p "$$(dirname $@)"
	$(LD) build/shell.c.o $(LIBC_OBJECT_FILES) $(LDFLAGS) -o $@

build/loop: Makefile linker.ld build/loop.c.o $(LIBC_OBJECT_FILES)
	mkdir -p "$$(dirname $@)"
	$(LD) build/loop.c.o $(LIBC_OBJECT_FILES) $(LDFLAGS) -o $@

build/echo: Makefile linker.ld build/echo.c.o $(LIBC_OBJECT_FILES)
	mkdir -p "$$(dirname $@)"
	$(LD) build/echo.c.o $(LIBC_OBJECT_FILES) $(LDFLAGS) -o $@

build/fault: Makefile linker.ld build/fault.c.o $(LIBC_OBJECT_FILES)
	mkdir -p "$$(dirname $@)"
	$(LD) build/fault.c.o $(LIBC_OBJECT_FILES) $(LDFLAGS) -o $@

build/clear: Makefile linker.ld build/clear.c.o $(LIBC_OBJECT_FILES)
	mkdir -p "$$(dirname $@)"
	$(LD) build/clear.c.o $(LIBC_OBJECT_FILES) $(LDFLAGS) -o $@

build/touch: Makefile linker.ld build/touch.c.o $(LIBC_OBJECT_FILES)
	mkdir -p "$$(dirname $@)"
	$(LD) build/touch.c.o $(LIBC_OBJECT_FILES) $(LDFLAGS) -o $@

build/ls: Makefile linker.ld build/ls.c.o $(LIBC_OBJECT_FILES)
	mkdir -p "$$(dirname $@)"
	$(LD) build/ls.c.o $(LIBC_OBJECT_FILES) $(LDFLAGS) -o $@

build/mkdir: Makefile linker.ld build/mkdir.c.o $(LIBC_OBJECT_FILES)
	mkdir -p "$$(dirname $@)"
	$(LD) build/mkdir.c.o $(LIBC_OBJECT_FILES) $(LDFLAGS) -o $@

build/increment: Makefile linker.ld build/increment.c.o $(LIBC_OBJECT_FILES)
	mkdir -p "$$(dirname $@)"
	$(LD) build/increment.c.o $(LIBC_OBJECT_FILES) $(LDFLAGS) -o $@

build/hd: Makefile linker.ld build/hd.c.o $(LIBC_OBJECT_FILES)
	mkdir -p "$$(dirname $@)"
	$(LD) build/hd.c.o $(LIBC_OBJECT_FILES) $(LDFLAGS) -o $@

build/truncate: Makefile linker.ld build/truncate.c.o $(LIBC_OBJECT_FILES)
	mkdir -p "$$(dirname $@)"
	$(LD) build/truncate.c.o $(LIBC_OBJECT_FILES) $(LDFLAGS) -o $@

build/cat: Makefile linker.ld build/cat.c.o $(LIBC_OBJECT_FILES)
	mkdir -p "$$(dirname $@)"
	$(LD) build/cat.c.o $(LIBC_OBJECT_FILES) $(LDFLAGS) -o $@

build/test.sh: src/test.sh
	mkdir -p "$$(dirname $@)"
	cp src/test.sh build/test.sh

build/diff: Makefile linker.ld build/diff.c.o $(LIBC_OBJECT_FILES)
	mkdir -p "$$(dirname $@)"
	$(LD) build/diff.c.o $(LIBC_OBJECT_FILES) $(LDFLAGS) -o $@

build/ps: Makefile linker.ld build/ps.c.o $(LIBC_OBJECT_FILES)
	mkdir -p "$$(dirname $@)"
	$(LD) build/ps.c.o $(LIBC_OBJECT_FILES) $(LDFLAGS) -o $@

build/kill: Makefile linker.ld build/kill.c.o $(LIBC_OBJECT_FILES)
	mkdir -p "$$(dirname $@)"
	$(LD) build/kill.c.o $(LIBC_OBJECT_FILES) $(LDFLAGS) -o $@

build/sleep: Makefile linker.ld build/sleep.c.o $(LIBC_OBJECT_FILES)
	mkdir -p "$$(dirname $@)"
	$(LD) build/sleep.c.o $(LIBC_OBJECT_FILES) $(LDFLAGS) -o $@

build/mount: Makefile linker.ld build/mount.c.o $(LIBC_OBJECT_FILES)
	mkdir -p "$$(dirname $@)"
	$(LD) build/mount.c.o $(LIBC_OBJECT_FILES) $(LDFLAGS) -o $@

# Compilation rules for *.s files.
build/%.s.o: src/%.s Makefile
	mkdir -p "$$(dirname $@)"
	$(AS) $(ASFLAGS) -c $< -o $@

# Compilation rules for *.c files.
build/%.c.o: src/%.c Makefile
	mkdir -p "$$(dirname $@)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Remove object files and the final executable.
.PHONY: clean
clean:
	rm -rf build

