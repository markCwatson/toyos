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

    printk(path);

    struct process* process = 0;
    res = process_load_switch(path, &process);
    if (res < 0) {
        goto out;
    }

    task_switch(process->task);
    task_return(&process->task->registers);

out:
    return 0;
}