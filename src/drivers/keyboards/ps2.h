/**
 * @file ps2.h
 * @brief PS/2 keyboard controller interface definitions.
 * 
 * This file contains definitions and function declarations for interacting with a PS/2 keyboard controller.
 * The PS/2 controller is a hardware device used to interface with keyboards and mice, commonly found in older computer systems.
 * More information about the PS/2 keyboard can be found at https://wiki.osdev.org/PS/2_Keyboard.
 */

#ifndef _PS2_KEYBOARD_H_
#define _PS2_KEYBOARD_H_

/**
 * @brief PS/2 controller I/O port addresses.
 * 
 * These constants define the I/O port addresses used to communicate with the PS/2 controller.
 * 
 * For more information on the PS/2 controller, see: https://wiki.osdev.org/%228042%22_PS/2_Controller
 */
#define PS2_PORT                        0x64  /**< The I/O port address for the PS/2 controller. */
#define PS2_COMMAND_ENABLE_FIRST_PORT   0xae  /**< Command to enable the first PS/2 port (usually used for keyboard). */
#define PS2_KEYBOARD_INPUT_PORT         0x60  /**< The I/O port address for the PS/2 keyboard input. */

/**
 * @brief PS/2 keyboard interrupt definitions.
 */
#define PS2_ISR_KEYBOARD_INTERRUPT      0x21  /**< Keyboard interrupt number. */

/**
 * @brief PS/2 keyboard scancode definitions.
 */
#define PS2_KEYBOARD_KEY_RELEASED       0x80  /**< Bitmask for key release events. */
#define PS2_KEYBOARD_CAPSLOCK           0x3a  /**< Scancode for the caps lock key. */
#define PS2_KEYBOARD_ARROW_UP           0x48  /**< Scancode for the up arrow key. */
#define PS2_KEYBOARD_ARROW_DOWN         0x50  /**< Scancode for the down arrow key. */

/**
 * @brief Registers the PS/2 keyboard with the keyboard system.
 * 
 * This function registers the PS/2 keyboard with the keyboard system, allowing it to be used for input.
 * 
 * @return 0 on success, error code on failure.
 */
int ps2_register(void);

#endif
