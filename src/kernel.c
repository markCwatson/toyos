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
 */
void printk(const char* str) {
    size_t len = strlen(str);

    for (int i = 0; i < len; i++) {
        terminal_writechar(str[i], 15);  // Color 15 is typically white text on a black background
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
    printk(str);
    while (1);  // Infinite loop to halt the system
}

/**
 * @brief Entry point for the kernel after booting.
 *
 * This function initializes various subsystems of the kernel, including the terminal,
 * heap, file system, disk, and interrupt descriptor table (IDT). It sets up paging,
 * enables interrupts, and optionally runs tests if compiled in test mode.
 */
void kernel_main(void) {
    terminal_init();
    printk("Terminal initialized!\n");

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

    printk("\nKernel initialized!\n");

    while (1);
}
