#include "process.h"
#include "task/task.h"
#include "task/process.h"
#include "stdlib/string.h"
#include "status.h"
#include "config.h"
#include "kernel.h"
#include "stdlib/printf.h"

/**
 * System command handler for loading and starting a new process.
 * 
 * This function is called when the system command SYSTEM_CMD6_PROCESS_LOAD_START is invoked.
 * 
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void* sys_cmd6_process_load_start(struct interrupt_frame* frame) {
    void* filename_user_ptr = task_get_stack_item(task_current(), 0);
    char filename[TOYOS_MAX_PATH];
    int res = task_copy_string_from_task(task_current(), filename_user_ptr, filename, sizeof(filename));
    if (res < 0) {
        goto out;
    }

    char path[TOYOS_MAX_PATH];
    strcat(path, "0:/");
    strcat(path, filename);
    strcat(path, ".elf");

    struct process* process = 0;
    res = process_load_switch(path, &process);
    if (res < 0) {
        goto out;
    }

    task_switch(process->task);
    task_return(&process->task->registers);

out:
    return NULL;
}

/**
 * System command handler for exiting the current process.
 * 
 * This function is called when the system command SYSTEM_CMD7_PROCESS_EXIT is invoked.
 * 
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void* sys_cmd7_process_exit(struct interrupt_frame* frame) {
    struct process* process = task_current()->process;
    process_terminate(process);
    task_next();
    return NULL;
}

/**
 * System command handler for getting the program arguments.
 * 
 * This function is called when the system command SYSTEM_CMD8_GET_PROGRAM_ARGUMENTS is invoked.
 * 
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void* sys_cmd8_get_program_arguments(struct interrupt_frame* frame) {
    struct process* process = task_current()->process;
    struct process_arguments* arguments = task_virtual_address_to_physical(task_current(), task_get_stack_item(task_current(), 0));

    process_get_arguments(process, &arguments->argc, &arguments->argv);
    return 0;
}

/**
 * System command handler for invoking a system command.
 * 
 * This function is called when the system command SYSTEM_CMD9_INVOKE_SYSTEM_CMD is invoked.
 * 
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void* sys_cmd9_invoke_system_cmd(struct interrupt_frame* frame) {
    struct command_argument* arguments = task_virtual_address_to_physical(task_current(), task_get_stack_item(task_current(), 0));
    if (!arguments || strlen(arguments[0].argument) == 0) {
        return ERROR(-EINVARG);
    }

    struct command_argument* root_command_argument = &arguments[0];
    const char* program_name = root_command_argument->argument;

    char path[TOYOS_MAX_PATH];
    strcat(path, "0:/");
    strcat(path, program_name);
    strcat(path, ".elf");

    struct process* process = 0;
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
    task_return(&process->task->registers);

    return 0;
}

/**
 * System command handler for fetching the process list.
 * 
 * This function is called when the system command SYSTEM_CMD11_GET_PROCESSES is invoked.
 * It returns a list of processes.
 * 
 * @warning The memory for the list is allocated from the current process's memory space
 * and must be freed by the caller.
 * 
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void* sys_cmd11_get_processes(struct interrupt_frame* frame) {
    struct process_info* info = (struct process_info*)process_malloc(task_current()->process,
                                                                     sizeof(struct process_info) * TOYOS_MAX_PROCESSES);
    if (!info) {
        return ERROR(-ENOMEM);
    }

    // keep separate index for info array (note: not mapped 1:1 with pid)
    int index = 0;

    for (int pid = 0; pid < TOYOS_MAX_PROCESSES; pid++) {
        // indicates no process
        info[index].id = -1;

        struct process* process = process_get(pid);
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

/**
 * System command handler for forking a new process.
 * 
 * This function is called when the system command SYSTEM_CMD12_FORK is invoked.
 * 
 * @param frame The interrupt frame.
 * @return The PID of the child process on success or an error code on failure.
 */
void* sys_cmd12_fork(struct interrupt_frame* frame) {
    struct process* parent = task_current()->process;
    struct process* child = NULL;
    int res = process_fork(parent, &child);
    if (res < 0) {
        return ERROR(res);
    }

    task_switch(child->task);
    task_return(&child->task->registers);

    return (void*)((uint32_t)child->id);
}

/**
 * System command handler for waiting for a process to exit.
 * 
 * This function is called when the system command SYSTEM_CMD13_WAIT is invoked.
 * 
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void* sys_cmd13_wait(struct interrupt_frame* frame) {
    struct process* process = task_current()->process;
    int res = process_wait(process);
    if (res < 0) {
        return ERROR(res);
    }

    return NULL;
}

/**
 * System command handler for killing a process.
 * 
 * This function is called when the system command SYSTEM_CMD14_KILL is invoked.
 * 
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void* sys_cmd14_kill(struct interrupt_frame* frame) {
    int pid = (int)task_get_stack_item(task_current(), 0);
    struct process* process = process_get(pid);
    if (!process) {
        return ERROR(-ENOENT);
    }

    process_terminate(process);
    return NULL;
}

/**
 * System command handler for executing a new process.
 * 
 * This function is called when the system command SYSTEM_CMD15_EXEC is invoked.
 * 
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void* sys_cmd15_exec(struct interrupt_frame* frame) {
    // Get the filename from the stack
    void* filename_user_ptr = task_get_stack_item(task_current(), 0);
    char filename[TOYOS_MAX_PATH];
    int res = task_copy_string_from_task(task_current(), filename_user_ptr, filename, sizeof(filename));
    if (res < 0) {
        goto out;
    }

    char path[TOYOS_MAX_PATH] = "0:/";
    strcat(path, filename);
    strcat(path, ".elf");

    struct process* current_process = task_current()->process;

    // Load the new program
    res = process_load_replace(current_process, path);
    if (res < 0) {
        goto out;
    }

    // Switch to the new program
    task_switch(current_process->task);
    task_return(&current_process->task->registers);

out:
    return NULL;
}
