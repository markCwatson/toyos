#ifndef _TERMINAL_H_
#define _TERMINAL_H_

// Defines the width and height of the terminal screen in character cells
#define VGA_WIDTH 80  /**< The number of character columns on the screen. */
#define VGA_HEIGHT 25 /**< The number of character rows on the screen. */

// Cursor ports
#define VGA_CMD_PORT 0x3d4  /**< The I/O port for the VGA command register. */
#define VGA_DATA_PORT 0x3d5 /**< The I/O port for the VGA data register. */

// Cursor positions
#define VGA_CURSOR_HIGH 0x0e /**< The high byte of the cursor position. */
#define VGA_CURSOR_LOW 0x0f  /**< The low byte of the cursor position. */

// Defines for terminal colors
#define VGA_COLOR_BLACK 0
#define VGA_COLOR_BLUE 1
#define VGA_COLOR_GREEN 2
#define VGA_COLOR_CYAN 3
#define VGA_COLOR_RED 4
#define VGA_COLOR_MAGENTA 5
#define VGA_COLOR_BROWN 6
#define VGA_COLOR_LIGHT_GREY 7
#define VGA_COLOR_DARK_GREY 8
#define VGA_COLOR_LIGHT_BLUE 9
#define VGA_COLOR_LIGHT_GREEN 10
#define VGA_COLOR_LIGHT_CYAN 11
#define VGA_COLOR_LIGHT_RED 12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_LIGHT_BROWN 14
#define VGA_COLOR_WHITE 15

/**
 * @brief Initializes the terminal interface.
 *
 * This function sets up the terminal for text output. It typically initializes the screen,
 * sets the default text color, clears the screen, and sets the cursor position to the top-left corner.
 */
void terminal_init(void);

/**
 * @brief Clears the terminal screen.
 *
 * This function clears the entire terminal screen by writing spaces with the default color
 * to each character cell.
 */
void terminal_clear_all(void);

/**
 * @brief Deletes the last character written to the terminal.
 *
 * This function moves the cursor back by one position and writes a space character
 * to erase the last character written to the terminal.
 */
void terminal_backspace(void);

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
void terminal_writechar(char c, unsigned char fg, unsigned char bg);

/**
 * @brief Updates the hardware cursor to the current position.
 *
 * This function sets the VGA hardware cursor to the position specified by
 * the global variables terminal_row and terminal_col.
 */
void terminal_update_cursor(void);

#endif
