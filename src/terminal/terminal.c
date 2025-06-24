#include "terminal.h"
#include "io/io.h"
#include "stdlib/string.h"
#include <stddef.h>
#include <stdint.h>

// Base address of the VGA video memory
static uint16_t *video_mem = NULL;

// Current position of the cursor in terms of row and column
static uint16_t terminal_row = 0;
static uint16_t terminal_col = 0;

// Screen buffer
static uint16_t screen_buffer[VGA_HEIGHT][VGA_WIDTH];

/**
 * @brief Reads the current cursor position from the VGA hardware.
 *
 * This function reads the current cursor position from the VGA hardware and returns
 * the position as a 16-bit value.
 *
 * @return The current cursor position.
 */
uint16_t terminal_get_cursor_position(void) {
    uint16_t pos = 0;
    outb(VGA_CMD_PORT, 0x0f);
    pos |= insb(VGA_DATA_PORT);
    outb(VGA_CMD_PORT, 0x0e);
    pos |= ((uint16_t)insb(VGA_DATA_PORT)) << 8;
    return pos;
}

/**
 * @brief Disables the VGA hardware cursor.
 */
static void terminal_disable_cursor(void) {
    outb(VGA_CMD_PORT, 0x0a);
    outb(VGA_DATA_PORT, 0x20);
}

/**
 * @brief Enables the VGA hardware cursor.
 *
 * This function enables the VGA hardware cursor by setting the cursor start and end
 * scanlines. The cursor start and end values are the scanlines that define the cursor shape.
 *
 * @example values of 15 and 15 create an underline cursor.
 *
 * @param cursor_start The scanline where the cursor starts.
 * @param cursor_end The scanline where the cursor ends.
 */
static void terminal_enable_cursor(uint8_t cursor_start, uint8_t cursor_end) {
    // Set cursor start scanline register
    outb(VGA_CMD_PORT, 0x0a);
    outb(VGA_DATA_PORT, (insb(VGA_DATA_PORT) & 0xc0) | cursor_start);

    // Set cursor end scanline register
    outb(VGA_CMD_PORT, 0x0b);
    outb(VGA_DATA_PORT, (insb(VGA_DATA_PORT) & 0xe0) | cursor_end);
}

/**
 * @brief Combines a character and its color attribute into a single value.
 *
 * This function creates a 16-bit value combining a character (in the lower 8 bits)
 * and its color attribute (in the upper 8 bits). This format is used by the VGA text mode.
 *
 * @param c The character to display.
 * @param color The color attribute for the character.
 * @return A 16-bit value representing the character and its color.
 */
static uint16_t terminal_make_char(char c, char color) {
    return (color << 8) | c;
}

/**
 * @brief Places a character at a specific position on the screen.
 *
 * This function writes a character with the specified color at the given
 * (x, y) coordinates in the terminal.
 *
 * @param x The column position.
 * @param y The row position.
 * @param c The character to display.
 * @param color The color attribute for the character.
 */
static void terminal_putchar(int x, int y, char c, unsigned char color) {
    video_mem[(y * VGA_WIDTH) + x] = terminal_make_char(c, color);
}

/**
 * @brief Places a character at a specific position in the buffer.
 *
 * This function writes a character with the specified color at the given
 * (x, y) coordinates in the screen buffer.
 *
 * @param x The column position.
 * @param y The row position.
 * @param c The character to display.
 * @param color The color attribute for the character.
 */
static void terminal_buffer_putchar(int x, int y, char c, unsigned char color) {
    screen_buffer[y][x] = terminal_make_char(c, color);
}

/**
 * @brief Updates the VGA video memory with the screen buffer contents.
 */
static void terminal_update_vga_memory(void) {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            video_mem[(y * VGA_WIDTH) + x] = screen_buffer[y][x];
        }
    }
}

/**
 * @brief Scrolls the screen buffer up by one row.
 */
static void terminal_scroll(void) {
    // Copy each row to the row above it
    for (int y = 1; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            screen_buffer[y - 1][x] = screen_buffer[y][x];
        }
    }

    // Clear the last line
    for (int x = 0; x < VGA_WIDTH; x++) {
        screen_buffer[VGA_HEIGHT - 1][x] = terminal_make_char(' ', (VGA_COLOR_BLUE << 4) | VGA_COLOR_WHITE);
    }

    terminal_row = VGA_HEIGHT - 1;
}

void terminal_update_cursor(void) {
    uint16_t position = terminal_row * VGA_WIDTH + terminal_col;

    // Send the low byte of the cursor position
    outb(VGA_CMD_PORT, VGA_CURSOR_LOW);
    outb(VGA_DATA_PORT, (uint8_t)(position & 0xff));

    // Send the high byte of the cursor position
    outb(VGA_CMD_PORT, VGA_CURSOR_HIGH);
    outb(VGA_DATA_PORT, (uint8_t)((position >> 8) & 0xff));
}

void terminal_writechar(char c, unsigned char fg, unsigned char bg) {
    if (c == '\n') {
        terminal_row += 1;
        terminal_col = 0;

        if (terminal_row >= VGA_HEIGHT) {
            terminal_scroll();
        }

        goto update;
    }

    if (c == 0x08) {
        terminal_backspace();
        goto update;
    }

    terminal_buffer_putchar(terminal_col, terminal_row, c, ((bg & 0x0f) << 4) | (fg & 0x0f));
    terminal_col += 1;

    // If the cursor reaches the end of the line, move to the next line
    if (terminal_col >= VGA_WIDTH) {
        terminal_col = 0;
        terminal_row += 1;

        // If the cursor reaches the end of the screen, scroll the screen
        if (terminal_row >= VGA_HEIGHT) {
            terminal_scroll();
        }
    }

update:
    terminal_update_vga_memory();
}

void terminal_backspace(void) {
    if (terminal_row == 0 && terminal_col == 0) {
        return;
    }

    if (terminal_col == 0) {
        terminal_row -= 1;
        terminal_col = VGA_WIDTH;
    }

    terminal_col -= 1;
    terminal_writechar(' ', VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    terminal_col -= 1;
}

void terminal_clear_all(void) {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            terminal_buffer_putchar(x, y, ' ', ((VGA_COLOR_BLUE & 0x0f) << 4) | (VGA_COLOR_BLUE & 0x0f));
            video_mem[(y * VGA_WIDTH) + x] = screen_buffer[y][x];
        }
    }

    terminal_row = 0;
    terminal_col = 0;
}

void terminal_init(void) {
    // VGA text mode memory address starts at 0xb8000
    video_mem = (uint16_t *)(0xb8000);
    terminal_enable_cursor(15, 15);
    terminal_clear_all();
}
