#!/bin/bash

cd "$(dirname "$0")"

[ -f ovmf_vars.fd ] || cp firmware/OVMF_VARS.fd ovmf_vars.fd
[ -f disk.img ] || ./mkdisk.sh
[ -f slave.img ] || truncate -s 16M slave.img

qemu-system-x86_64 \
  -drive if=pflash,format=raw,readonly=on,file=firmware/OVMF_CODE.fd \
  -drive if=pflash,format=raw,file=ovmf_vars.fd \
  -drive file=disk.img,format=raw,if=ide,index=0 \
  -drive file=slave.img,format=raw,if=ide,index=1 \
  -cdrom x86_32_os.iso \
  -serial stdio \
  -s -S
