#!/bin/bash

# tung tung tung sa-rewrite

cd "$(dirname "$0")"

DISK_SECTORS=$((64 * 2048))
P1_START=2048
P1_SIZE=$((48 * 2048))
P2_START=$((P1_START + P1_SIZE))
P2_SIZE=$((DISK_SECTORS - P2_START))

SLAVE_SECTORS=$((16 * 2048))
SP1_START=2048
SP1_SIZE=$((SLAVE_SECTORS - SP1_START))

rm -f disk.img slave.img
truncate -s 64M disk.img
truncate -s 16M slave.img

printf 'label: dos\n%s,%s,83\n%s,%s,83\n' "$P1_START" "$P1_SIZE" "$P2_START" "$P2_SIZE" | sfdisk -q disk.img
printf 'label: dos\n%s,%s,83\n' "$SP1_START" "$SP1_SIZE" | sfdisk -q slave.img

if [ -d disk_root ]; then
  mkdir -p disk_root/mnt/extra disk_root/mnt/slave
  mke2fs -q -t ext2 -b 4096 -O ^dir_index -L deprecatOS -d disk_root -E offset=$((P1_START * 512)) disk.img $((P1_SIZE / 8))
else
  mke2fs -q -t ext2 -b 4096 -O ^dir_index -L deprecatOS -E offset=$((P1_START * 512)) disk.img $((P1_SIZE / 8))
fi

mke2fs -q -t ext2 -b 4096 -O ^dir_index -L extra -E offset=$((P2_START * 512)) disk.img $((P2_SIZE / 8))
mke2fs -q -t ext2 -b 4096 -O ^dir_index -L slave -E offset=$((SP1_START * 512)) slave.img $((SP1_SIZE / 8))
