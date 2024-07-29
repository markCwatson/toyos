#ifndef _SYS_H_
#define _SYS_H_

/**
 * @brief Enumeration of system commands that can be invoked using interrupt 0x80.
 */
enum SYSTEM_COMMANDS {
    SYSTEM_COMMAND0_TEST,
    SYSTEM_COMMAND1_PRINT,
    SYSTEM_COMMAND2_GETKEY,
    SYSTEM_COMMAND3_PUTCHAR
};

/**
 * @brief Registers the system commands that can be invoked using interrupt 0x80.
 */
void sys_register_commands(void);

#endif