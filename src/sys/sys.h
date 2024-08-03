#ifndef _SYS_H_
#define _SYS_H_

/**
 * @brief Enumeration of system commands that can be invoked using interrupt 0x80.
 */
enum SYSTEM_COMMANDS {
    SYSTEM_COMMAND0_TEST,
    SYSTEM_COMMAND1_PRINT,
    SYSTEM_COMMAND2_GETKEY,
    SYSTEM_COMMAND3_PUTCHAR,
    SYSTEM_COMMAND4_MALLOC,
    SYSTEM_COMMAND5_FREE,
    SYSTEM_COMMAND6_PROCESS_LOAD_START,
    SYSTEM_COMMAND7_EXIT,
    SYSTEM_COMMAND8_GET_PROGRAM_ARGUMENTS,
    SYSTEM_COMMAND9_INVOKE_SYSTEM_COMMAND,
    SYSTEM_COMMAND10_CLEAR_TERMINAL,
    SYSTEM_COMMAND11_GET_PROCESSES,
};

/**
 * @brief Registers the system commands that can be invoked using interrupt 0x80.
 */
void sys_register_commands(void);

#endif