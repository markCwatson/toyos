#include "keyboard.h"
#include "kernel.h"
#include "status.h"
#include "task/process.h"
#include "task/task.h"

static struct keyboard *keyboard_list_head = NULL;
static struct keyboard *keyboard_list_last = NULL;

void keyboard_init(void) {
    struct keyboard *keyboard = keyboard_list_head;
    while (keyboard) {
        keyboard->init();
        keyboard = keyboard->next;
    }
}

int keyboard_insert(struct keyboard *keyboard) {
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
static int keyboard_get_tail_index(struct process *process) {
    if (!process) {
        return -EINVARG;
    }

    return process->keyboard.tail % sizeof(process->keyboard.buffer);
}

void keyboard_backspace(struct process *process) {
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

void keyboard_push(char c) {
    struct process *process = process_current();
    if (!process) {
        return;
    }

    int real_index = keyboard_get_tail_index(process);
    process->keyboard.buffer[real_index] = c;
    process->keyboard.tail++;
}

char keyboard_pop(void) {
    if (!task_current()) {
        return 0;
    }

    struct process *process = task_current()->process;
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

void keyboard_set_capslock(struct keyboard *keyboard, keyboard_capslock_state state) {
    keyboard->capslock_state = state;
}

keyboard_capslock_state keyboard_get_capslock(struct keyboard *keyboard) {
    return keyboard->capslock_state;
}
