# Nuke built-in rules and variables.
MAKEFLAGS += -rR
.SUFFIXES:

# Convenience macro to reliably declare user overridable variables.
override USER_VARIABLE = $(if $(filter $(origin $(1)),default undefined),$(eval override $(1) := $(2)))

# Install prefix; /usr/local is a good, standard default pick.
$(call USER_VARIABLE,PREFIX,/usr/local)

# User controllable C compiler command.
$(call USER_VARIABLE,KCC,cc)

# User controllable assembler command (must be clang)
$(call USER_VARIABLE,KAS,cc)

# User controllable linker command.
$(call USER_VARIABLE,KLD,ld)

# User controllable C flags.
$(call USER_VARIABLE,KCFLAGS,-g -O0 -pipe)

# User controllable C preprocessor flags. We set none by default.
$(call USER_VARIABLE,KCPPFLAGS,)

# User controllable linker flags. We set none by default.
$(call USER_VARIABLE,KLDFLAGS,)

override KCFLAGS += \
    -target x86_64-unknown-none \
    -Wall \
    -Wextra \
    -Werror=implicit-function-declaration \
    -std=gnu11 \
    -nostdinc \
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
    -mcmodel=kernel

override KCPPFLAGS := \
    -I src \
    -isystem freestnd-c-hdrs-0bsd \
    $(KCPPFLAGS) \
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
    -mno-sse2

override KLDFLAGS += \
    -m elf_x86_64 \
    -nostdlib \
    -static \
    -z max-page-size=0x1000 \
    -gc-sections \
    -T linker.ld

# Use "find" to glob all *.c, *.s, and *.asm files in the tree and obtain the
# object and header dependency file names.
override CFILES := $(shell cd src && find -L * -type f -name '*.c' | LC_ALL=C sort)
override ASFILES := $(shell cd src && find -L * -type f -name '*.s' | LC_ALL=C sort)

override OBJ := $(addprefix build/,$(CFILES:.c=.c.o) $(ASFILES:.s=.s.o))
override HEADER_DEPS := $(addprefix build/,$(CFILES:.c=.c.d) $(ASFILES:.s=.s.d))

# Default target.
.PHONY: all
all: build/persistos.iso

# Link rules for the final executable.
build/persistos: Makefile linker.ld $(OBJ)
	mkdir -p "$$(dirname $@)"
	$(KLD) $(OBJ) $(KLDFLAGS) -o $@

# Include header dependencies.
-include $(HEADER_DEPS)

# Compilation rules for *.c files.
build/%.c.o: src/%.c Makefile
	mkdir -p "$$(dirname $@)"
	$(KCC) $(KCFLAGS) $(KCPPFLAGS) -c $< -o $@

# Compilation rules for *.s files.
build/%.s.o: src/%.s Makefile
	mkdir -p "$$(dirname $@)"
	$(KAS) $(KASFLAGS) -c $< -o $@

build/persistos.iso: build/persistos build/initrd.bin
	rm -rf iso_root
	mkdir -p iso_root/boot
	cp -v build/persistos iso_root/boot/
	cp -v build/initrd.bin iso_root/boot/initrd.bin
	mkdir -p iso_root/boot/limine
	cp -v limine.conf iso_root/boot/limine/
	mkdir -p iso_root/EFI/BOOT
	cp -v limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-uefi-cd.bin iso_root/boot/limine/
	cp -v limine/BOOTX64.EFI iso_root/EFI/BOOT/
	cp -v limine/BOOTIA32.EFI iso_root/EFI/BOOT/
	xorriso -as mkisofs -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o build/persistos.iso
	./limine/limine bios-install build/persistos.iso
	rm -rf iso_root

.PHONY: limine
limine:
	$(MAKE) -C limine

# Remove object files and the final executable.
.PHONY: clean
clean:
	rm -rf build

