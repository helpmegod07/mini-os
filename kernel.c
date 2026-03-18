#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Hardware text mode color constants. */
enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
	return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
	return (uint16_t) uc | (uint16_t) color << 8;
}

size_t strlen(const char* str) {
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;

void terminal_initialize(void) {
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	terminal_buffer = (uint16_t*) 0xB8000;
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

void terminal_setcolor(uint8_t color) {
	terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_row = 0; // Simple wrap-around, no scrolling for now
        }
        return;
    }
    
    if (c == '\b') {
        if (terminal_column > 0) {
            terminal_column--;
        } else if (terminal_row > 0) {
            terminal_row--;
            terminal_column = VGA_WIDTH - 1;
        }
        terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
        return;
    }

	terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == VGA_WIDTH) {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT)
			terminal_row = 0;
	}
}

void terminal_write(const char* data, size_t size) {
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}

void terminal_writestring(const char* data) {
	terminal_write(data, strlen(data));
}

// I/O Ports
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port) );
    return ret;
}

// Keyboard
// Simplified US QWERTY keyboard map (scancode 1)
unsigned char kbdus[128] =
{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '`',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '/',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

char keyboard_read_char() {
    uint8_t scancode;
    while (1) {
        if (inb(0x64) & 1) {
            scancode = inb(0x60);
            if (!(scancode & 0x80)) { // Key press
                return kbdus[scancode];
            }
        }
    }
}

// Very simple editor state
#define EDITOR_MAX_LINES 20
#define EDITOR_MAX_COLS 78
char editor_buffer[EDITOR_MAX_LINES][EDITOR_MAX_COLS];
int editor_cursor_x = 0;
int editor_cursor_y = 0;
bool in_editor = false;

void clear_screen() {
    terminal_row = 0;
    terminal_column = 0;
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

void draw_editor() {
    clear_screen();
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        terminal_putentryat(' ', terminal_color, x, 0);
    }
    terminal_row = 0;
    terminal_column = 0;
    terminal_writestring(" MINI EDITOR - Press ESC to exit");
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    
    for (int y = 0; y < EDITOR_MAX_LINES; y++) {
        for (int x = 0; x < EDITOR_MAX_COLS; x++) {
            char c = editor_buffer[y][x];
            if (c == 0) break;
            terminal_putentryat(c, terminal_color, x, y + 2);
        }
    }
    
    // Update cursor position visually (blink effect not implemented here, just positioning)
    terminal_column = editor_cursor_x;
    terminal_row = editor_cursor_y + 2;
}

void run_editor() {
    in_editor = true;
    
    // Initialize empty buffer if not already done
    static bool editor_init = false;
    if (!editor_init) {
        for (int y = 0; y < EDITOR_MAX_LINES; y++) {
            for (int x = 0; x < EDITOR_MAX_COLS; x++) {
                editor_buffer[y][x] = 0;
            }
        }
        editor_init = true;
    }
    
    draw_editor();
    
    while (in_editor) {
        char c = keyboard_read_char();
        
        if (c == 27) { // ESC
            in_editor = false;
            break;
        } else if (c == '\b') { // Backspace
            if (editor_cursor_x > 0) {
                editor_cursor_x--;
                editor_buffer[editor_cursor_y][editor_cursor_x] = 0;
            } else if (editor_cursor_y > 0) {
                editor_cursor_y--;
                // Find end of previous line
                editor_cursor_x = 0;
                while (editor_buffer[editor_cursor_y][editor_cursor_x] != 0 && editor_cursor_x < EDITOR_MAX_COLS) {
                    editor_cursor_x++;
                }
            }
        } else if (c == '\n') { // Enter
            if (editor_cursor_y < EDITOR_MAX_LINES - 1) {
                editor_cursor_y++;
                editor_cursor_x = 0;
            }
        } else if (c >= 32 && c <= 126) { // Printable chars
            if (editor_cursor_x < EDITOR_MAX_COLS - 1) {
                editor_buffer[editor_cursor_y][editor_cursor_x] = c;
                editor_cursor_x++;
                editor_buffer[editor_cursor_y][editor_cursor_x] = 0; // Null terminate
            }
        }
        
        draw_editor();
    }
    
    clear_screen();
    terminal_writestring("Exited editor.\n");
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

void shell() {
    char input_buffer[256];
    int input_idx = 0;
    
    terminal_writestring("Welcome to MiniOS!\n");
    terminal_writestring("Type 'help' for commands, 'edit' for the text editor.\n");
    
    while (1) {
        terminal_writestring("> ");
        input_idx = 0;
        
        while (1) {
            char c = keyboard_read_char();
            
            if (c == '\n') {
                terminal_putchar('\n');
                input_buffer[input_idx] = '\0';
                break;
            } else if (c == '\b') {
                if (input_idx > 0) {
                    input_idx--;
                    terminal_putchar('\b');
                }
            } else if (c >= 32 && c <= 126 && input_idx < 255) {
                input_buffer[input_idx++] = c;
                terminal_putchar(c);
            }
        }
        
        if (input_idx > 0) {
            if (strcmp(input_buffer, "help") == 0) {
                terminal_writestring("Available commands:\n");
                terminal_writestring("  help  - Show this message\n");
                terminal_writestring("  clear - Clear the screen\n");
                terminal_writestring("  edit  - Open the built-in text editor\n");
            } else if (strcmp(input_buffer, "clear") == 0) {
                clear_screen();
            } else if (strcmp(input_buffer, "edit") == 0) {
                run_editor();
            } else {
                terminal_writestring("Unknown command: ");
                terminal_writestring(input_buffer);
                terminal_writestring("\n");
            }
        }
    }
}

void kernel_main(void) {
	/* Initialize terminal interface */
	terminal_initialize();
    
    /* Start the simple shell */
    shell();
}
