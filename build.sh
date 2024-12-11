#!/bin/bash
set -e

if [ ! -L "uefi" ]; then
    ln -s posix-uefi/uefi
fi
make
dd if=/dev/zero of=disk.img bs=1k count=1440
mformat -i disk.img -f 1440 ::
mmd -i disk.img ::/EFI
mmd -i disk.img ::/EFI/BOOT
mcopy -i disk.img BOOTX64.EFI ::/EFI/BOOT

qemu-system-x86_64 disk.img -bios uefi-image.fd -accel kvm
