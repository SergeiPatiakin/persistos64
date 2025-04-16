## Walkthrough: get firmware
Download OVMF.fd from e.g. https://github.com/clearlinux/common/blob/master/OVMF.fd

## Walkthrough: prepare a raw drive image on macOS host
```bash
# Initialize hard drive image
dd if=/dev/zero of=em/hd.img bs=512 count=100000

# Run a debian docker image to use parted
docker run -it --mount type=bind,source="$(pwd)"/em,target=/em debian

# Inside the docker container, install packages
apt update
apt install parted exfatprogs

# Run parted
parted /em/hd.img
unit s

# Make a partition table
mklabel gpt

# Make first partition
# Note: 20127s means make the partition up to sector 20127 inclusive
mkpart primary 128s 20127s

# Make second partition
mkpart primary 20128s 40127s

# Exit gparted

# Format the exfat partition
mkfs.exfat /em/part.img

# Exit Docker container

# Mount on host
hdiutil attach em/part.img

# Note: this will print mount point like /Volume/Untitled

# Add files on host
pushd /Volumes/Untitled
mkdir text pictures music
echo 'Lorem ipsum' > text/lorem
popd

# Copy partition image into disk image
dd if=em/part.img of=em/hd2.img seek=128 bs=512 conv=notrunc
```

## Walkthrough: compile QEMU
```bash
pip3 install sphinx
pip3 install sphinx-rtd-theme
brew install ninja
./configure
make
```

## Walkthrough: run QEMU with GDB server

On host:
```bash
qemu-system-x86_64 -S -s -m 512 -nic user \
-boot menu=on \
-drive if=none,id=usbstick,format=raw,file=kernel/build/persistos.iso \
-usb -device usb-ehci,id=ehci -device usb-storage,bus=ehci.0,drive=usbstick \
-drive file=em/hd.img,if=none,id=nvm,format=raw \
-device nvme,serial=deadbeef,drive=nvm \
-bios em/OVMF.fd --no-shutdown --no-reboot -monitor stdio \
-global i8042.kbd-throttle=on
```

## Walkthrough: debug kernel with GDB

In Docker:
```bash
cd /code
gdb kernel/build/persistos
# In gdb shell:
target remote host.docker.internal:1234
# Set breakpoints here as desired
b some_func
# Start boot
c
```

Alternatively, as a one-liner:
```bash
gdb kernel/build/persistos -ex 'target remote host.docker.internal:1234' -ex c
```

## Walkthrough: debug userspace program with GDB
```bash
gdb userspace/build/shell -ex 'target remote host.docker.internal:1234' -ex c
```
