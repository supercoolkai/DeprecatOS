set -e

rm -rf build
rm -rf iso

cmake -B build -G "Unix Makefiles"

gcc -m32 -ffreestanding -fno-pie \
  -fno-stack-protector -c user/shell/shell.c -o build/shell.o -I user
gcc -m32 -ffreestanding -fno-pie \
  -fno-stack-protector -c user/shell/crt0.s -o build/crt0.o -I user
gcc -m32 -ffreestanding -fno-pie \
  -fno-stack-protector -c user/shell/sys/syscall.c -o build/syscall.o -I user
ld -m elf_i386 -T user/shell/shell.ld build/crt0.o \
  build/shell.o build/syscall.o -o build/shell.elf
objcopy -O binary build/shell.elf build/shell.bin

cmake --build build

mkdir -p iso/boot/grub
cp build/deprecatos.elf iso/boot/
cp src/grub.cfg iso/boot/grub/
grub-mkrescue -o deprecatos.iso iso
