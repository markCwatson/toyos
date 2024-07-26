#ifdef RUN_TESTS
#include "../tests/tests.h"
#endif
#include "kernel.h"
#include "terminal/terminal.h"
#include "idt/idt.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include "disk/disk.h"
#include "string/string.h"
#include "disk/streamer.h"
#include "fs/file.h"

// Pointer to the 4GB paging chunk used by the kernel
struct paging_4gb_chunk *kernel_chunk = NULL;

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
 */
void panick(const char* str) {
    printk(str, VGA_COLOR_WHITE, VGA_COLOR_RED);
    while (1);  // Infinite loop to halt the system
}

/**
 * @brief Prints an alert message to the terminal.
 * 
 * This function prints an alert message to the terminal using a fixed color attribute.
 * 
 * @param str The null-terminated string to print.
 */
void alertk(const char* str) {
    printk(str, VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
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

    kheap_init();
    fs_init();
    disk_search_and_init();
    idt_init();

    // Set up paging for the kernel
    kernel_chunk = paging_new_4gb(PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    paging_switch(kernel_chunk);
    enable_paging();

    // Enable interrupts
    enable_int();

#ifdef RUN_TESTS
    // Run tests if the kernel is compiled in test mode
    tests_run();
#endif

    printk("\nKernel initialized!\n", VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    while (1);
}
