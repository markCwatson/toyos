#ifndef TERMINAL_H
#define TERMINAL_H

// Defines the width and height of the terminal screen in character cells
#define VGA_WIDTH   80  /**< The number of character columns on the screen. */
#define VGA_HEIGHT  20  /**< The number of character rows on the screen. */

/**
 * @brief Initializes the terminal interface.
 *
 * This function sets up the terminal for text output. It typically initializes the screen,
 * sets the default text color, clears the screen, and sets the cursor position to the top-left corner.
 */
void terminal_init(void);

/**
 * @brief Writes a character to the terminal with a specified color.
 *
 * This function places a character at the current cursor position on the terminal screen
 * using the specified color attribute. It handles screen scrolling if necessary.
 *
 * @param c The character to write to the terminal.
 * @param color The color attribute for the character, typically combining foreground and background colors.
 */
void terminal_writechar(char c, char color);

#endif
