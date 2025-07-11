#include "process.h"
#include "config.h"
#include "idt/idt.h"
#include "kernel.h"
#include "locks/spinlock.h"
#include "status.h"
#include "stdlib/printf.h"
#include "stdlib/string.h"
#include "task/process.h"
#include "task/task.h"

/**
 * @brief Spinlock for active processes.
 *
 * Used for parent processes who wait for child processes to finish (ex. shell).
 */
static struct spinlock_t lock = {
    .locked = 0,
};

void *sys_command6_process_load_start(struct interrupt_frame *frame) {
    void *filename_user_ptr = task_get_stack_item(task_current(), 0);
    char filename[TOYOS_MAX_PATH];
    int res = copy_string_from_task(task_current(), filename_user_ptr, filename, sizeof(filename));
    if (res < 0) {
        goto out;
    }

    char path[TOYOS_MAX_PATH];
    strcat(path, "0:/");
    strcat(path, filename);
    strcat(path, ".elf");

    struct process *process = 0;
    res = process_load_switch(path, &process);
    if (res < 0) {
        goto out;
    }

    task_switch(process->task);
    task_return(&process->task->registers);

out:
    return NULL;
}

void *sys_command7_process_exit(struct interrupt_frame *frame) {
    struct process *process = task_current()->process;
    process_terminate(process);
    task_next();
    return NULL;
}

void *sys_command8_get_program_arguments(struct interrupt_frame *frame) {
    struct process *process = task_current()->process;
    struct process_arguments *arguments =
        task_virtual_address_to_physical(task_current(), task_get_stack_item(task_current(), 0));

    process_get_arguments(process, &arguments->argc, &arguments->argv);
    return 0;
}

void *sys_command9_invoke_system_command(struct interrupt_frame *frame) {
    struct command_argument *arguments =
        task_virtual_address_to_physical(task_current(), task_get_stack_item(task_current(), 0));
    if (!arguments || strlen(arguments[0].argument) == 0) {
        return ERROR(-EINVARG);
    }

    struct command_argument *root_command_argument = &arguments[0];
    const char *program_name = root_command_argument->argument;

    char path[TOYOS_MAX_PATH];
    strcat(path, "0:/");
    strcat(path, program_name);
    strcat(path, ".elf");

    struct process *process = 0;
    int res = process_load_switch(path, &process);
    if (res < 0) {
        alertk("Command not recognized.\n\n");
        return ERROR(res);
    }

    res = process_inject_arguments(process, root_command_argument);
    if (res < 0) {
        return ERROR(res);
    }

    task_switch(process->task);
    spin_lock(&lock);
    task_return(&process->task->registers);

    // Should never reach here: should be in user mode for new process by now
    panick("task_switch failed\n");
    return 0;
}

void *sys_command11_get_processes(struct interrupt_frame *frame) {
    struct process_info *info = (struct process_info *)process_malloc(
        task_current()->process, sizeof(struct process_info) * TOYOS_MAX_PROCESSES);
    if (!info) {
        return ERROR(-ENOMEM);
    }

    // keep separate index for info array (note: not mapped 1:1 with pid)
    int index = 0;

    for (int pid = 0; pid < TOYOS_MAX_PROCESSES; pid++) {
        // indicates no process
        info[index].id = -1;

        struct process *process = process_get(pid);
        if (!process) {
            goto next;
        }

        info[index].id = process->id;
        strcpy(info[index].filename, process->filename);

    next:
        index += 1;
    }

    return info;
}

void *sys_command12_check_lock(struct interrupt_frame *frame) {
    return lock.locked ? ERROR(-EBUSY) : OK;
}

void *sys_command13_done(struct interrupt_frame *frame) {
    spin_unlock(&lock);
    return 0;
}

void *sys_command14_fork(struct interrupt_frame *frame) {
    struct process *child = NULL;
    int res = process_fork(&child);
    if (res < 0) {
        return ERROR(res);
    }

    return (void *)(uintptr_t)child->id;
}
