#!/usr/bin/env bash
# Invoked by Make. Assumes dependencies are already built
rm -rf build/iso_root
mkdir -p build/iso_root/boot
cp -v build/persistos build/iso_root/boot/
cp -v build/initrd.bin build/iso_root/boot/initrd.bin
mkdir -p build/iso_root/boot/limine
cp -v limine.conf build/iso_root/boot/limine/
mkdir -p build/iso_root/EFI/BOOT
cp -v limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-uefi-cd.bin build/iso_root/boot/limine/
cp -v limine/BOOTX64.EFI build/iso_root/EFI/BOOT/
cp -v limine/BOOTIA32.EFI build/iso_root/EFI/BOOT/
xorriso -as mkisofs -b boot/limine/limine-bios-cd.bin \
    -no-emul-boot -boot-load-size 4 -boot-info-table \
    --efi-boot boot/limine/limine-uefi-cd.bin \
    -efi-boot-part --efi-boot-image --protective-msdos-label \
    build/iso_root -o build/persistos.iso
./limine/limine bios-install build/persistos.iso
# rm -rf build/iso_root
