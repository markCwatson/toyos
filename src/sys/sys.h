#ifndef _SYS_H_
#define _SYS_H_

/**
 * Enumeration of system commands that can be invoked using interrupt 0x80.
 */
enum SYSTEM_CMDS {
    SYSTEM_CMD0_TEST,
    SYSTEM_CMD1_PRINT,
    SYSTEM_CMD2_GETKEY,
    SYSTEM_CMD3_PUTCHAR,
    SYSTEM_CMD4_MALLOC,
    SYSTEM_CMD5_FREE,
    SYSTEM_CMD6_PROCESS_LOAD_START,
    SYSTEM_CMD7_EXIT,
    SYSTEM_CMD8_GET_PROGRAM_ARGUMENTS,
    SYSTEM_CMD9_INVOKE_SYSTEM_CMD,
    SYSTEM_CMD10_CLEAR_TERMINAL,
    SYSTEM_CMD11_GET_PROCESSES,
    SYSTEM_CMD12_FORK,
    SYSTEM_CMD13_WAIT,
    SYSTEM_CMD14_KILL,
    SYSTEM_CMD15_EXEC
};

/**
 * Registers the system commands that can be invoked using interrupt 0x80.
 */
void sys_register_commands(void);

#endif