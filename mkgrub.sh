cd build
make -j32
cd ..
cp build/kernel/arch/amd64/BOOT_SHIM grub/boot/BOOT_SHIM
grub-mkrescue grub -o grub.iso
