# MiniOS Makefile
# Run 'make' to build the ISO, 'make run' to test in QEMU, 'make clean' to clean up.

CC      = gcc
CFLAGS  = -m32 -ffreestanding -fno-pie -fno-pic -O2 -Wall -Wextra
LDFLAGS = -m32 -T src/linker.ld -ffreestanding -fno-pie -fno-pic -O2 -nostdlib

OBJS = boot.o kernel.o

all: myos.iso

boot.o: src/boot.s
	$(CC) $(CFLAGS) -c src/boot.s -o boot.o

kernel.o: src/kernel.c
	$(CC) $(CFLAGS) -c src/kernel.c -o kernel.o

iso/boot/myos.bin: $(OBJS)
	$(CC) $(LDFLAGS) -o iso/boot/myos.bin $(OBJS) -lgcc

myos.iso: iso/boot/myos.bin
	grub-mkrescue -o myos.iso iso/

run: myos.iso
	qemu-system-i386 -cdrom myos.iso -m 32 -nographic

clean:
	rm -f *.o iso/boot/myos.bin myos.iso

.PHONY: all run clean
