#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

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
    keyboard_init_fp init;   /**< Function pointer to the keyboard initialization function. */
    char name[20];           /**< The name of the keyboard device. */
    struct keyboard* next;   /**< Pointer to the next keyboard in a linked list. */
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

#endif
