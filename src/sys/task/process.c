#include "process.h"
#include "task/task.h"
#include "task/process.h"
#include "string/string.h"
#include "status.h"
#include "config.h"
#include "kernel.h"

/**
 * @brief System command handler for loading and starting a new process.
 * 
 * This function is called when the system command SYSTEM_COMMAND6_PROCESS_LOAD_START is invoked.
 * 
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void* sys_command6_process_load_start(struct interrupt_frame* frame) {
    void* filename_user_ptr = task_get_stack_item(task_current(), 0);
    char filename[TOYOS_MAX_PATH];
    int res = copy_string_from_task(task_current(), filename_user_ptr, filename, sizeof(filename));
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
 * @brief System command handler for exiting the current process.
 * 
 * This function is called when the system command SYSTEM_COMMAND7_PROCESS_EXIT is invoked.
 * 
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void* sys_command7_process_exit(struct interrupt_frame* frame) {
    struct process* process = task_current()->process;
    process_terminate(process);
    task_next();
    return NULL;
}

/**
 * @brief System command handler for getting the program arguments.
 * 
 * This function is called when the system command SYSTEM_COMMAND8_GET_PROGRAM_ARGUMENTS is invoked.
 * 
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void* sys_command8_get_program_arguments(struct interrupt_frame* frame) {
    struct process* process = task_current()->process;
    struct process_arguments* arguments = task_virtual_address_to_physical(task_current(), task_get_stack_item(task_current(), 0));

    process_get_arguments(process, &arguments->argc, &arguments->argv);
    return 0;
}

/**
 * @brief System command handler for invoking a system command.
 * 
 * This function is called when the system command SYSTEM_COMMAND9_INVOKE_SYSTEM_COMMAND is invoked.
 * 
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void* sys_command9_invoke_system_command(struct interrupt_frame* frame) {
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
