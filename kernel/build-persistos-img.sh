#!/usr/bin/env bash
# Invoked by Make. Assumes dependencies are already built
set -euo pipefail

# Populate iso_root
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

DISK_SIZE="64MiB"
PART_START_MIB=1
PART_START_SECTOR=$((PART_START_MIB * 1024 * 1024 / 512))

mkdir -p build
rm -f build/persistos.img build/persistos.fat

# Create the disk image and its single MBR/FAT32 partition.
echo "Creating disk image..."
truncate -s "$DISK_SIZE" build/persistos.img

echo "Creating MBR partition table..."
parted -s build/persistos.img \
    mklabel msdos \
    mkpart primary fat32 "${PART_START_MIB}MiB" 100% \
    set 1 boot on

# Determine partition size from the partition table.
PART_END_SECTOR=$(
    parted -ms build/persistos.img unit s print |
        awk -F: '$1 == "1" {
            gsub("s", "", $3)
            print $3
        }'
)

PART_SECTORS=$((PART_END_SECTOR - PART_START_SECTOR + 1))
PART_SIZE=$((PART_SECTORS * 512))

echo "Partition:"
echo "  start:  ${PART_START_SECTOR} sectors"
echo "  size:   ${PART_SIZE} bytes"

# Create a standalone FAT32 filesystem of exactly the partition size.
echo "Creating FAT32 filesystem..."
truncate -s "$PART_SIZE" build/persistos.fat
mkfs.fat -F 32 build/persistos.fat

# Populate the FAT32 filesystem without mounting it.
# Copy everything from iso_root into the FAT filesystem.
# mcopy's -s recursively copies directories.
echo "Populating FAT32 filesystem..."
MTOOLS_SKIP_CHECK=1 mcopy -i build/persistos.fat -s build/iso_root/* ::

# Copy the FAT32 filesystem into partition 1.
echo "Writing FAT32 filesystem into partition..."
dd if=build/persistos.fat \
   of=build/persistos.img \
   bs=512 \
   seek="$PART_START_SECTOR" \
   conv=notrunc \
   status=none

# Install Limine
echo "Installing Limine BIOS bootloader..."
./limine/limine bios-install build/persistos.img

