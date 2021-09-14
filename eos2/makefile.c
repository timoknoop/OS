#  *******************************************************
#  ***                                                 ***
#  ***   EOS Makefile (c) 2020-2021 by Elmer Hoeksema  ***
#  ***                                                 ***
#  *******************************************************
# 
#  Copyright (c) 2020-2021, Elmer Hoeksema
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#      * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

.PHONY: all build clean

build/rapi-boot/eos-init.bin: build/obj/init.o build/obj/dev-gpio.o
	aarch64-linux-gnu-gcc -o eos-init.elf -nostdlib -fno-pie -nostartfiles -ffreestanding -T linker.ld -e start build/obj/init.o build/obj/dev-gpio.o
	aarch64-linux-gnu-objcopy eos-init.elf -O binary -j .eos-* build/rapi-boot/eos-init.bin
	rm eos-init.elf

build/obj/init.o: src/init.c src/eos.h 
	aarch64-linux-gnu-gcc -o build/obj/init.o -c -fno-pie -ffreestanding src/init.c\

build/obj/dev-gpio.o: src/dev-gpio.c src/dev-gpio.h 
	aarch64-linux-gnu-gcc -o build/obj/dev-gpio.o -c -fno-pie -ffreestanding src/dev-gpio.c\

build: build/rapi-boot/eos-init.bin
	dd if=/dev/zero of=build/eos.img count=100 bs=1M
	sudo losetup -D
	sudo losetup -fP build/eos.img
	printf "o\nn\np\n1\n\n\nt\nc\nw\n" | sudo fdisk /dev/loop0
	sudo losetup -o 1048576 /dev/loop1 /dev/loop0
	sudo mkfs.vfat -n EOS /dev/loop1
	sudo mount /dev/loop1 build/mnt
	sudo cp -r build/rapi-boot/* build/mnt/.
	sudo umount build/mnt
	sudo losetup -D

clean:
	if [ -f "build/rapi-boot/eos-init.bin" ]; then rm build/rapi-boot/eos-init.bin; fi
	if [ -f "build/eos.img" ]; then rm build/eos.img; fi
	if [ -f "build/obj/init.o" ]; then rm build/obj/*.o; fi
