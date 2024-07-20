#include "kernel.h"
#include "terminal/terminal.h"
#include "idt/idt.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include "disk/disk.h"
#include "string/string.h"
#include "disk/streamer.h"

static struct paging_4gb_chunk *kernel_chunk = 0;

void printk(const char* str)
{
    size_t len = strlen(str);

    for (int i = 0; i < len; i++)
    {
        terminal_writechar(str[i], 15);
    }
}

void kernel_main()
{
    terminal_init();
    printk("Terminal initialized!\n");

    kheap_init();
    disk_search_and_init();
    idt_init();

    kernel_chunk = paging_new_4gb(PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    paging_switch(kernel_chunk);
    enable_paging();

    enable_int();

    struct disk_stream *stream = streamer_new(0);
    streamer_seek(stream, 0x201);
    unsigned char c = 0;
    streamer_read(stream, &c, 1);

    while (1);

    printk("\nGood bye!");
}
