# MiniOS

A minimal 32-bit x86 operating system written in C and Assembly. Boots via GRUB from a USB drive or ISO image. Includes a simple interactive shell and a built-in visual text editor.

---

## Project Structure

```
myos/
├── Makefile              # Build system
├── README.md             # This file
├── src/
│   ├── boot.s            # Assembly entry point (Multiboot header + stack setup)
│   ├── kernel.c          # Entire kernel: VGA display, keyboard, shell, editor
│   └── linker.ld         # Linker script (places kernel at 1 MiB)
└── iso/
    └── boot/
        ├── myos.bin      # Compiled kernel binary (generated)
        └── grub/
            └── grub.cfg  # GRUB boot menu configuration
```

---

## How to Build

### Requirements

Install the following on Ubuntu/Debian:

```bash
sudo apt-get install nasm gcc gcc-multilib grub-pc-bin grub-common xorriso qemu-system-x86
```

### Build the ISO

```bash
make
```

This produces `myos.iso` in the project root.

### Test in QEMU (virtual machine)

```bash
make run
```

### Write to a USB Drive

Replace `/dev/sdX` with your actual USB device (use `lsblk` to find it):

```bash
sudo dd if=myos.iso of=/dev/sdX bs=4M status=progress && sync
```

Then boot your PC from the USB drive (select it in your BIOS/UEFI boot menu).

---

## Shell Commands

Once booted, you will see a `>` prompt. Available commands:

| Command | Description                        |
|---------|------------------------------------|
| `help`  | Show the list of available commands |
| `clear` | Clear the screen                   |
| `edit`  | Open the built-in text editor      |

---

## Built-in Text Editor

Type `edit` at the shell prompt to open the editor.

- **Type** any printable character to insert text at the cursor.
- **Enter** moves to the next line.
- **Backspace** deletes the character before the cursor.
- **ESC** exits the editor and returns to the shell.

The editor supports up to **20 lines** of **78 characters** each. The buffer is preserved between editor sessions during the same boot.

---

## How to Modify the OS

All kernel logic lives in a single file: `src/kernel.c`. It is intentionally kept small and readable. Key sections:

| Section in kernel.c       | What it does                                     |
|---------------------------|--------------------------------------------------|
| `enum vga_color` + helpers | Defines VGA text-mode colors and character cells |
| `terminal_*` functions    | Low-level screen writing (writes to `0xB8000`)   |
| `kbdus[]` + `keyboard_read_char()` | PS/2 keyboard scancode-to-ASCII mapping |
| `run_editor()` + `draw_editor()` | The visual text editor                    |
| `shell()`                 | The command-line shell loop                      |
| `kernel_main()`           | Entry point called by `boot.s`                   |

After editing, simply run `make` to rebuild and `make run` to test.

---

## Architecture Notes

- **Bootloader**: GRUB 2 with Multiboot specification.
- **CPU mode**: 32-bit protected mode (set up by GRUB before `kernel_main` is called).
- **Display**: VGA text mode at physical address `0xB8000` (80×25 characters).
- **Keyboard**: Polled PS/2 keyboard via I/O ports `0x60` and `0x64`.
- **No OS dependencies**: Compiled with `-ffreestanding -nostdlib`; no libc is used.
