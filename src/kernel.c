#ifdef RUN_TESTS
#include "../tests/tests.h"
#endif
#include "kernel.h"
#include "terminal/terminal.h"
#include "idt/idt.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include "disk/disk.h"
#include "string/string.h"
#include "disk/streamer.h"
#include "fs/file.h"
#include "gdt/gdt.h"
#include "config.h"
#include "task/tss.h"
#include "task/task.h"
#include "task/process.h"
#include "stdlib/printf.h"
#include "sys/sys.h"

// Pointer to the 4GB paging chunk used by the kernel
struct paging_4gb_chunk *kernel_chunk = NULL;

// Task state segment (TSS) for the kernel
struct tss tss;

// Global descriptor table (GDT) entries
struct gdt gdt_real[TOYOS_TOTAL_GDT_SEGMENTS];
struct gdt_structured gdt_structured[TOYOS_TOTAL_GDT_SEGMENTS] = {
    {.base = 0, .limit = 0, .type = 0},                             // Null segment
    {.base = 0, .limit = 0xffffffff, .type = 0x9a},                 // Kernel code segment
    {.base = 0, .limit = 0xffffffff, .type = 0x92},                 // Kernel data segment
    {.base = 0, .limit = 0xffffffff, .type = 0xf8},                 // User code segment
    {.base = 0, .limit = 0xffffffff, .type = 0xf2},                 // User data segment
    {.base = (uintptr_t)&tss, .limit = sizeof(tss), .type = 0xe9}   // Task state segment
};

/**
 * @brief Prints a string to the terminal.
 *
 * This function writes each character of the given string to the terminal, using a fixed
 * color attribute. It is typically used for kernel-level logging and debugging.
 *
 * @param str The null-terminated string to print.
 * @param fg The foreground color of the text.
 * @param bg The background color of the text.
 */
void printk(const char* str, unsigned char fg, unsigned char bg) {
    size_t len = strlen(str);

    for (int i = 0; i < len; i++) {
        terminal_writechar(str[i], fg, bg);
    }
}

/**
 * @brief Displays a panic message and halts the system.
 *
 * This function prints a panic message to the terminal and then enters an infinite loop,
 * effectively halting the system. It is used in critical error situations where the kernel
 * cannot safely continue execution.
 *
 * @param str The null-terminated string describing the panic reason.
 * @param ... The optional arguments to format the string.
 */
void panick(const char* str, ...) {
    va_list args;
    va_start(args, str);
    printf_colored(str, VGA_COLOR_RED, VGA_COLOR_BLACK, args);
    while (1);  // Infinite loop to halt the system
}

/**
 * @brief Prints an alert message to the terminal.
 *
 * This function prints an alert message to the terminal using a fixed color attribute.
 *
 * @param str The null-terminated string to print.
 * @param ... The optional arguments to format the string.
 */
void alertk(const char* str, ...) {
    va_list args;
    va_start(args, str);
    printf_colored(str, VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK, args);
}

/**
 * @brief Switches to the kernel page.
 * 
 * This function switches to the kernel page by setting up the kernel registers and
 * switching to the kernel chunk. This is used to switch to the kernel page when
 * the kernel is running, for example, when handling interrupts.
 */
void kernel_page(void) {
    kernel_registers();
    paging_switch(kernel_chunk);
}

/**
 * @brief Entry point for the kernel after booting.
 *
 * This function initializes various subsystems of the kernel, including the terminal,
 * heap, file system, disk, and interrupt descriptor table (IDT). It sets up paging,
 * enables interrupts, and optionally runs tests if compiled in test mode.
 */
void maink(void) {
    terminal_init();
    printk("Terminal initialized!\n", VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    // Initialize the global descriptor table (GDT)
    memset(gdt_real, 0, sizeof(gdt_real));
    gdt_structured_to_gdt(gdt_real, gdt_structured, TOYOS_TOTAL_GDT_SEGMENTS);
    gdt_load(gdt_real,sizeof(gdt_real));

    kheap_init();
    fs_init();
    disk_search_and_init();
    idt_init();

    // Setup the task state segment (TSS)
    memset(&tss, 0, sizeof(tss));
    tss.esp0 = 0x60000;             // Set the stack pointer for ring 0
    tss.ss0 = TOYOS_DATA_SELECTOR;  // Set the stack segment for ring 0

    // Load the TSS
    tss_load(0x28);

    // Set up paging for the kernel
    kernel_chunk = paging_new_4gb(PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    paging_switch(kernel_chunk);
    enable_paging();

    // Initialize the sys system call handlers for system calls
    sys_register_commands();

    printk("\nKernel initialized!\n", VGA_COLOR_WHITE, VGA_COLOR_BLACK);

#ifdef RUN_TESTS
    // Run tests if the kernel is compiled in test mode
    enable_interrupt();
    tests_run();
    disable_interrupt();
#endif

    // Load the first process
    // \todo: move this to the tests and replace with a proper first process
    struct process* process = NULL;
    int res = process_load("0:/test.bin", &process);
    if (ISERROR(res)) {
        panick("Failed to load the first process!\n");
    }

    printk("Running the first task!\n", VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    task_run_first_ever_task();

    // Halt the system if the first task returns
    panick("First task returned!\n");
}
