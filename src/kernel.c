#include "kernel.h"
#include "terminal/terminal.h"
#include "idt/idt.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include "disk/disk.h"
#include "string/string.h"
#include "disk/streamer.h"
#include "fs/file.h"

static struct paging_4gb_chunk *kernel_chunk = 0;

void printk(const char* str) {
    size_t len = strlen(str);

    for (int i = 0; i < len; i++) {
        terminal_writechar(str[i], 15);
    }
}

void kernel_main() {
    terminal_init();
    printk("Terminal initialized!\n");

    kheap_init();

    fs_init();

    disk_search_and_init();
    idt_init();

    kernel_chunk = paging_new_4gb(PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    paging_switch(kernel_chunk);
    enable_paging();

    enable_int();

    int fd = fopen("0:/test.txt", "r");
    if (fd) {
        printk("File opened!\n");
    }

    int fd2 = fopen("0:/test2.txt", "r");
    if (fd2 == 0) {
        printk("File not found!\n");
    }

    while (1);
}
