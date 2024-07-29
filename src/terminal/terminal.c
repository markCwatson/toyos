#include "terminal.h"
#include "string/string.h"
#include <stddef.h>
#include <stdint.h>

// Base address of the VGA video memory
static uint16_t* video_mem = 0;

// Current position of the cursor in terms of row and column
static uint16_t terminal_row = 0;
static uint16_t terminal_col = 0;

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
 * @brief Writes a character to the terminal at the current cursor position.
 *
 * This function places a character at the current cursor position, updating
 * the cursor position afterwards. It handles newline characters by moving
 * the cursor to the beginning of the next line.
 *
 * @param c The character to write.
 * @param fg The foreground color of the character.
 * @param bg The background color of the character.
 */
void terminal_writechar(char c, unsigned char fg, unsigned char bg) {
    if (c == '\n') {
        terminal_row += 1;
        terminal_col = 0;
        return;
    }

    if (c == 0x08) {
        terminal_backspace();
        return;
    }

    terminal_putchar(terminal_col, terminal_row, c, ((bg & 0x0f) << 4) | (fg & 0x0f));
    terminal_col += 1;

    // If the cursor reaches the end of the line, move to the next line
    if (terminal_col >= VGA_WIDTH) {
        terminal_col = 0;
        terminal_row += 1;
    }
}

/**
 * @brief Deletes the last character written to the terminal.
 * 
 * This function moves the cursor back by one position and writes a space character
 * to erase the last character written to the terminal.
 */
void terminal_backspace(void) {
    if (terminal_row == 0 && terminal_col == 0) {
        return;
    }

    if (terminal_col == 0) {
        terminal_row -= 1;
        terminal_col = VGA_WIDTH;
    }

    terminal_col -=1;
    terminal_writechar(' ', VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    terminal_col -=1;
}

/**
 * @brief Initializes the terminal.
 *
 * This function sets up the terminal for text output by initializing
 * the video memory pointer and clearing the screen. It sets the cursor
 * to the top-left corner.
 */
void terminal_init(void) {
    video_mem = (uint16_t*)(0xb8000); // VGA text mode memory address
    terminal_row = 0;
    terminal_col = 0;

    // Clear the terminal by writing spaces with the default color
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            terminal_putchar(x, y, ' ', 0);
        }
    }   
}
