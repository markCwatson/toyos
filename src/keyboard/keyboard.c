#include "keyboard.h"
#include "status.h"
#include "kernel.h"
#include "task/process.h"
#include "task/task.h"

static struct keyboard* keyboard_list_head = NULL;
static struct keyboard* keyboard_list_last = NULL;

/**
 * @brief Initializes the keyboard system.
 * 
 * This function initializes all keyboard devices in the linked list of keyboard devices.
 */
void keyboard_init(void) {
    struct keyboard* keyboard = keyboard_list_head;
    while (keyboard) {
        keyboard->init();
        keyboard = keyboard->next;
    }
}

/**
 * @brief Inserts a keyboard device into the linked list of keyboard devices.
 * @param keyboard The keyboard device to insert.
 * @return 0 on success, error code on failure.
 */
int keyboard_insert(struct keyboard* keyboard) {
    if (!keyboard) {
        return -EINVARG;
    }

    int res = OK;

    if (keyboard->init == NULL) {
        res = -EINVARG;
        goto out;
    }

    if (keyboard_list_last) {
        keyboard_list_last->next = keyboard;
    } else {
        keyboard_list_head = keyboard;
    }
    
    keyboard_list_last = keyboard;
    res = keyboard->init();

out:
    return res;
}

/**
 * @brief Retrieves the tail index of the keyboard buffer for the given process.
 * @param process The process for which to retrieve the tail index.
 * @return The tail index.
 */
static int keyboard_get_tail_index(struct process* process) {
    if (!process) {
        return -EINVARG;
    }

    return process->keyboard.tail % sizeof(process->keyboard.buffer);
}

/**
 * @brief Handles backspace functionality for the given process.
 * @param process The process for which to handle backspace.
 */
void keyboard_backspace(struct process* process) {
    if (!process) {
        return;
    }

    process->keyboard.tail -= 1;
    int real_index = keyboard_get_tail_index(process);
    if (real_index < 0) {
        return;
    }

    process->keyboard.buffer[real_index] = 0x00;
}

/**
 * @brief Pushes a character onto the keyboard buffer.
 * @param c The character to push onto the buffer.
 */
void keyboard_push(char c) {
    struct process* process = process_current();
    if (!process) {
        return;
    }

    int real_index = keyboard_get_tail_index(process);
    process->keyboard.buffer[real_index] = c;
    process->keyboard.tail++;
}

/**
 * @brief Pops a character from the keyboard buffer.
 * @return The character popped from the buffer.
 */
char keyboard_pop(void) {
    if (!task_current()) {
        return 0;
    }

    struct process* process = task_current()->process;
    int real_index = process->keyboard.head % sizeof(process->keyboard.buffer);
    char c = process->keyboard.buffer[real_index];
    if (c == 0x00) {
        // Nothing to pop return zero.
        return 0;
    }

    process->keyboard.buffer[real_index] = 0;
    process->keyboard.head += 1;
    return c;
}

/**
 * @brief Sets the state of the caps lock key for the given keyboard.
 * 
 * This function sets the state of the caps lock key for the given keyboard.
 * 
 * @param keyboard The keyboard device.
 * @param state The state of the caps lock key.
 */
void keyboard_set_capslock(struct keyboard* keyboard, keyboard_capslock_state state) {
    keyboard->capslock_state = state;
}

/**
 * @brief Retrieves the state of the caps lock key for the given keyboard.
 * 
 * This function retrieves the state of the caps lock key for the given keyboard.
 * 
 * @param keyboard The keyboard device.
 * @return The state of the caps lock key.
 */
keyboard_capslock_state keyboard_get_capslock(struct keyboard* keyboard) {
    return keyboard->capslock_state;
}
