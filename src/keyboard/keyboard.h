#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

// Flags for the caps lock key state
#define KEYBOARD_CAPS_LOCK_ON   1
#define KEYBOARD_CAPS_LOCK_OFF  0

/**
 * @typedef keyboard_capslock_state
 * @brief Represents the state of the caps lock key.
 */
typedef int keyboard_capslock_state;

struct process;

/**
 * @typedef keyboard_init_fp
 * @brief Function pointer type for keyboard initialization functions.
 */
typedef int (*keyboard_init_fp)(void);

/**
 * @struct keyboard
 * @brief Represents a keyboard device.
 */
struct keyboard {
    keyboard_init_fp init;                  /**< Function pointer to the keyboard initialization function. */
    char name[20];                          /**< The name of the keyboard device. */
    keyboard_capslock_state capslock_state; /**< The state of the caps lock key. */
    struct keyboard* next;                  /**< Pointer to the next keyboard in a linked list. */
};

/**
 * @brief Initializes the keyboard system.
 */
void keyboard_init(void);

/**
 * @brief Handles backspace functionality for the given process.
 * @param process The process for which to handle backspace.
 */
void keyboard_backspace(struct process* process);

/**
 * @brief Pushes a character onto the keyboard buffer.
 * @param c The character to push onto the buffer.
 */
void keyboard_push(char c);

/**
 * @brief Pops a character from the keyboard buffer.
 * @return The character popped from the buffer.
 */
char keyboard_pop(void);

/**
 * @brief Inserts a keyboard into the keyboard list.
 * @param keyboard The keyboard to insert.
 * @return 0 on success, error code on failure.
 */
int keyboard_insert(struct keyboard* keyboard);;

/**
 * @brief Sets the state of the caps lock key for the given keyboard.
 * 
 * This function sets the state of the caps lock key for the given keyboard.
 * 
 * @param keyboard The keyboard device.
 * @param state The state of the caps lock key.
 */
void keyboard_set_capslock(struct keyboard* keyboard, keyboard_capslock_state state);

/**
 * @brief Retrieves the state of the caps lock key for the given keyboard.
 * 
 * This function retrieves the state of the caps lock key for the given keyboard.
 * 
 * @param keyboard The keyboard device.
 * @return The state of the caps lock key.
 */
keyboard_capslock_state keyboard_get_capslock(struct keyboard* keyboard);

#endif
