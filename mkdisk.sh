#!/bin/bash

cd "$(dirname "$0")"

rm -f disk.img
truncate -s 64M disk.img
if [ -d disk_root ]; then
  mke2fs -q -t ext2 -b 4096 -L deprecatOS -d disk_root disk.img
else
  mke2fs -q -t ext2 -b 4096 -L deprecatOS disk.img
fi
