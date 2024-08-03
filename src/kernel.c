#include "kernel.h"
#include "terminal/terminal.h"
#include "idt/idt.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include "disk/disk.h"
#include "stdlib/string.h"
#include "disk/streamer.h"
#include "fs/file.h"
#include "gdt/gdt.h"
#include "config.h"
#include "task/tss.h"
#include "task/task.h"
#include "task/process.h"
#include "sys/sys.h"
#include "keyboard/keyboard.h"
#include "drivers/keyboards/ps2.h"
#include "stdlib/printf.h"

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
 * @brief Prints a string to the terminal using color attributes.
 *
 * This function writes each character of the given string to the terminal, using caller defined
 * color attribute. It is typically used for kernel-level logging and debugging.
 *
 * @param str The null-terminated string to print.
 * @param fg The foreground color of the text.
 * @param bg The background color of the text.
 */
void printk_colored(const char* str, unsigned char fg, unsigned char bg) {
    size_t len = strlen(str);

    for (int i = 0; i < len; i++) {
        terminal_writechar(str[i], fg, bg);
    }

    terminal_update_cursor();
}

/**
 * @brief Prints a string to the terminal.
 *
 * This function writes each character of the given string to the terminal, using a fixed
 * color attribute (white on black).
 * 
 * This function is a light-weight alternative to printf, and is used for kernel-level logging
 * and debugging.
 *
 * @param str The null-terminated string to print.
 */
void printk(const char* str) {
    printk_colored(str, VGA_COLOR_WHITE, VGA_COLOR_BLUE);
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
    printf_colored(str, VGA_COLOR_RED, VGA_COLOR_BLUE, args);
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
    printf_colored(str, VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLUE, args);
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
 * @brief Prints the "ToyOS" logo using ASCII art.
 *
 * This function displays the "ToyOS" name in a stylized ASCII art format.
 * The art uses simple characters to create a visually appealing representation of the OS name.
 */
static void print_toyos_logo(void) {
    const char* logo =
        "   _____              _  _     ___      ___   \n"
        "  |_   _|    ___     | || |   / _ \\    / __|  \n"
        "    | |     / _ \\     \\_, |  | (_) |   \\__ \\  \n"
        "   _|_|_    \\___/    _|__/    \\___/    |___/  \n"
        " _|\"\"\"\"\"| _|\"\"\"\"\"| _| \"\"\"\"| _|\"\"\"\"\"| _|\"\"\"\"\"| \n"
        " \"`-0-0-' \"`-0-0-' \"`-0-0-' \"`-0-0-' \"`-0-0-' version 0.0.0\n"
        "\n";

    printk(logo);
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
    printk_colored("ToyOS kernel starting...\n", VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE);

    // Initialize the global descriptor table (GDT)
    printk_colored("Initializing the GDT...\n", VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE);
    memset(gdt_real, 0, sizeof(gdt_real));
    gdt_structured_to_gdt(gdt_real, gdt_structured, TOYOS_TOTAL_GDT_SEGMENTS);
    gdt_load(gdt_real, sizeof(gdt_real));

    // Initialize the heap, file system, disk, and IDT
    printk_colored("Initializing the heap...\n", VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE);
    kheap_init();
    printk_colored("Initializing the file system...\n", VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE);
    fs_init();
    disk_search_and_init();
    printk_colored("Initializing the IDT...\n", VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE);
    idt_init();

    // Setup the task state segment (TSS)
    printk_colored("Setting up the TSS...\n", VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE);
    memset(&tss, 0, sizeof(tss));
    tss.esp0 = 0x60000;             // Set the stack pointer for ring 0
    tss.ss0 = TOYOS_DATA_SELECTOR;  // Set the stack segment for ring 0

    // Load the TSS
    tss_load(0x28);

    // Set up paging for the kernel
    printk_colored("Setting up paging...\n", VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE);
    kernel_chunk = paging_new_4gb(PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    paging_switch(kernel_chunk);
    enable_paging();

    // Initialize the sys system call handlers for system calls
    sys_register_commands();

    // Register the PS/2 keyboard driver
    printk_colored("Registering the PS/2 keyboard...\n", VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE);
    if (ps2_register() < 0) {
        panick("Failed to register the PS/2 keyboard!\n");
    }

    // Initialize the keyboard
    keyboard_init();

    // Print the ToyOS logo
    print_toyos_logo();
    // pause for one second
    for (int i = 0; i < 100000000; i++) {
        asm volatile("nop");
    }

    // Load the first process
    printk_colored("Loading the shell...\n", VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE);
    struct process* process = NULL;
    int res = process_load_switch("0:/shell.elf", &process);
    if (ISERROR(res)) {
        panick("Failed to load the shell!\n");
    }

    terminal_clear_all();
    task_run_first_ever_task();

    // Halt the system if the first task returns
    panick("First task returned!\n");
}
