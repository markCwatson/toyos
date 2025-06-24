/**
 * @file ps2.c
 * @brief PS/2 keyboard driver implementation.
 *
 * This file implements functions for initializing and handling input from a PS/2 keyboard.
 * It includes translation of scancodes to ASCII characters and setting up the keyboard for use.
 */

#include "ps2.h"
#include "idt/idt.h"
#include "io/io.h"
#include "kernel.h"
#include "keyboard/keyboard.h"
#include "status.h"
#include "task/task.h"
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Initializes the PS/2 keyboard.
 *
 * This function sends a command to the PS/2 controller to enable the first PS/2 port, which is typically used for the
 * keyboard.
 *
 * @return Returns OK (0) on success.
 */
int ps2_keyboard_init(void);

/**
 * @brief Converts a PS/2 keyboard scancode to an ASCII character.
 */
void ps2_keyboard_handle_interrupt(void);

/**
 * @brief Array mapping PS/2 scan codes to ASCII characters (Set 1).
 *
 * This array contains the mapping from scan codes to ASCII characters for the PS/2 keyboard's Set 1 scan code set.
 */
static uint8_t keyboard_scan_set_one[] = {
    0x00, 0x1b, '1', '2',  '3', '4',  '5',  '6',  '7',  '8',  '9',  '0',  '-',  '=',  0x08, '\t', 'Q',
    'W',  'E',  'R', 'T',  'Y', 'U',  'I',  'O',  'P',  '[',  ']',  0x0d, 0x00, 'A',  'S',  'D',  'F',
    'G',  'H',  'J', 'K',  'L', ';',  '\'', '`',  0x00, '\\', 'Z',  'X',  'C',  'V',  'B',  'N',  'M',
    ',',  '.',  '/', 0x00, '*', 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, '7',  '8', '9',  '-', '4',  '5',  '6',  '+',  '1',  '2',  '3',  '0',  '.'};

/**
 * @brief PS/2 keyboard structure definition.
 *
 * This structure defines the PS/2 keyboard, including its name and initialization function.
 */
struct keyboard ps2_keyboard = {.name = {"ps2"}, .init = ps2_keyboard_init};

/**
 * @brief Initializes the PS/2 keyboard.
 *
 * This function enables the first PS/2 port, preparing it for keyboard input.
 *
 * @return Returns OK (0) on successful initialization.
 */
int ps2_keyboard_init(void) {
    idt_register_interrupt_callback(PS2_ISR_KEYBOARD_INTERRUPT, ps2_keyboard_handle_interrupt);
    keyboard_set_capslock(&ps2_keyboard, KEYBOARD_CAPS_LOCK_OFF);

    // Enable the first PS/2 port
    outb(PS2_PORT, PS2_COMMAND_ENABLE_FIRST_PORT);
    return OK;
}

/**
 * @brief Converts a PS/2 keyboard scancode to an ASCII character.
 *
 * This function maps a given scancode to its corresponding ASCII character using the Set 1 scan code set.
 *
 * @param scancode The scancode received from the PS/2 keyboard.
 * @return The corresponding ASCII character, or 0 if the scancode is out of range.
 */
uint8_t ps2_keyboard_scancode_to_char(uint8_t scancode) {
    size_t size_of_keyboard_set_one = sizeof(keyboard_scan_set_one) / sizeof(uint8_t);
    if (scancode > size_of_keyboard_set_one) {
        return 0;
    }

    char c = keyboard_scan_set_one[scancode];
    if (keyboard_get_capslock(&ps2_keyboard) == KEYBOARD_CAPS_LOCK_OFF) {
        if (c >= 'A' && c <= 'Z') {
            c += 32;
        }
    }

    return c;
}

/**
 * @brief Handles PS/2 keyboard interrupts.
 */
void ps2_keyboard_handle_interrupt(void) {
    kernel_page();

    uint8_t scancode = 0;
    scancode = insb(PS2_KEYBOARD_INPUT_PORT);
    insb(PS2_KEYBOARD_INPUT_PORT);

    if (scancode & PS2_KEYBOARD_KEY_RELEASED) {
        return;
    }

    if (scancode == PS2_KEYBOARD_CAPSLOCK) {
        keyboard_capslock_state old_state = keyboard_get_capslock(&ps2_keyboard);
        keyboard_set_capslock(&ps2_keyboard,
                              old_state == KEYBOARD_CAPS_LOCK_ON ? KEYBOARD_CAPS_LOCK_OFF : KEYBOARD_CAPS_LOCK_ON);
    }

    uint8_t c = ps2_keyboard_scancode_to_char(scancode);
    if (c != 0) {
        keyboard_push(c);
    }

    task_page();
}

int ps2_register(void) {
    return keyboard_insert(&ps2_keyboard);
}
