set -e

rm -rf build
rm -rf iso

cmake -B build -G "Unix Makefiles"

user_cc() {
  gcc -m32 -ffreestanding -fno-pie \
    -fno-stack-protector -c "$1" -o "$2" -I user -I kernel -I etc/lib
}

user_cc user/shell/crt0.S build/crt0.o

user_objs=()
while IFS= read -r f; do
  o="build/${f%.c}.o"
  mkdir -p "$(dirname "$o")"
  user_cc "$f" "$o"
  user_objs+=("$o")
done < <((
  find user -name '*.c'
  find etc/lib -name '*.c'
) | sort)

ld -m elf_i386 -T user/shell/shell.ld build/crt0.o \
  "${user_objs[@]}" -o build/shell.elf
objcopy -O binary build/shell.elf build/shell.bin

cmake --build build

mkdir -p iso/boot/grub
cp build/deprecatos.elf iso/boot/
cp kernel/grub.cfg iso/boot/grub/
grub-mkrescue -o deprecatos.iso iso
