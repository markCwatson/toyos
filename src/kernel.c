#include "kernel.h"
#include "config.h"
#include "disk/disk.h"
#include "disk/streamer.h"
#include "drivers/keyboards/ps2.h"
#include "drivers/pci/pci.h"
#include "fs/file.h"
#include "gdt/gdt.h"
#include "idt/idt.h"
#include "keyboard/keyboard.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "memory/paging/paging.h"
#include "stdlib/printf.h"
#include "stdlib/string.h"
#include "sys/net/netdev.h"
#include "sys/sys.h"
#include "task/process.h"
#include "task/task.h"
#include "task/tss.h"
#include "terminal/terminal.h"

// Pointer to the 4GB paging chunk used by the kernel
struct paging_4gb_chunk *kernel_chunk = NULL;

// Task state segment (TSS) for the kernel
struct tss tss;

// Global descriptor table (GDT) entries
struct gdt gdt_real[TOYOS_TOTAL_GDT_SEGMENTS];
struct gdt_structured gdt_structured[TOYOS_TOTAL_GDT_SEGMENTS] = {
    {.base = 0, .limit = 0, .type = 0},                            // Null segment
    {.base = 0, .limit = 0xffffffff, .type = 0x9a},                // Kernel code segment
    {.base = 0, .limit = 0xffffffff, .type = 0x92},                // Kernel data segment
    {.base = 0, .limit = 0xffffffff, .type = 0xf8},                // User code segment
    {.base = 0, .limit = 0xffffffff, .type = 0xf2},                // User data segment
    {.base = (uintptr_t)&tss, .limit = sizeof(tss), .type = 0xe9}  // Task state segment
};

/**
 * @brief Prints the "ToyOS" logo using ASCII art.
 *
 * This function displays the "ToyOS" name in a stylized ASCII art format.
 * The art uses simple characters to create a visually appealing representation of the OS name.
 */
static void print_toyos_logo(void) {
    const char *logo = "   _____              _  _     ___      ___   \n"
                       "  |_   _|    ___     | || |   / _ \\    / __|  \n"
                       "    | |     / _ \\     \\_, |  | (_) |   \\__ \\  \n"
                       "   _|_|_    \\___/    _|__/    \\___/    |___/  \n"
                       " _|\"\"\"\"\"| _|\"\"\"\"\"| _| \"\"\"\"| _|\"\"\"\"\"| _|\"\"\"\"\"| \n"
                       " \"`-0-0-' \"`-0-0-' \"`-0-0-' \"`-0-0-' \"`-0-0-' version 0.0.0\n"
                       "\n";

    printk(logo);
}

void printk_colored(const char *str, unsigned char fg, unsigned char bg) {
    size_t len = strlen(str);

    for (int i = 0; i < len; i++) {
        terminal_writechar(str[i], fg, bg);
    }

    terminal_update_cursor();
}

void printk(const char *str) {
    printk_colored(str, VGA_COLOR_WHITE, VGA_COLOR_BLUE);
}

void panick(const char *str, ...) {
    va_list args;
    va_start(args, str);
    printf_colored(str, VGA_COLOR_RED, VGA_COLOR_BLUE, args);
    while (1)
        ;  // Infinite loop to halt the system
}

void alertk(const char *str, ...) {
    va_list args;
    va_start(args, str);
    printf_colored(str, VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLUE, args);
}

void kernel_page(void) {
    kernel_registers();
    paging_switch(kernel_chunk);
}

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

    keyboard_init();
    pci_enumerate_devices();

    // Initialize network interfaces
    printk_colored("Bringing up network interfaces...\n", VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE);
    int active_interfaces = netdev_bring_all_up();
    if (active_interfaces > 0) {
        printf("Successfully brought up %d network interface(s)\n", active_interfaces);
    } else {
        printf("No network interfaces were brought up\n");
    }

    print_toyos_logo();
    for (int i = 0; i < 100000000; i++) {
        asm volatile("nop");
    }

    // Load the first process
    printk_colored("Loading the shell...\n", VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE);
    struct process *process = NULL;
    int res = process_load_switch("0:/shell.elf", &process);
    if (ISERROR(res)) {
        panick("Failed to load the shell!\n");
    }

    terminal_clear_all();
    task_run_first_ever_task();

    // Halt the system if the first task returns
    panick("First task returned!\n");
}
