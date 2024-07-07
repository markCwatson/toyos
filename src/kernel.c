#include "kernel.h"
#include "idt/idt.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"

#include <stddef.h>
#include <stdint.h>

uint16_t* video_mem = 0;
uint16_t terminal_row = 0;
uint16_t terminal_col = 0;

uint16_t terminal_make_char(char c, char colour)
{
    return (colour << 8) | c;
}

void terminal_putchar(int x, int y, char c, char colour)
{
    video_mem[(y * VGA_WIDTH) + x] = terminal_make_char(c, colour);
}

void terminal_writechar(char c, char colour)
{
    if (c == '\n')
    {
        terminal_row += 1;
        terminal_col = 0;
        return;
    }

    terminal_putchar(terminal_col, terminal_row, c, colour);
    terminal_col += 1;

    if (terminal_col >= VGA_WIDTH)
    {
        terminal_col = 0;
        terminal_row += 1;
    }
}

void terminal_initialize()
{
    video_mem = (uint16_t*)(0xB8000);
    terminal_row = 0;
    terminal_col = 0;

    for (int y = 0; y < VGA_HEIGHT; y++)
    {
        for (int x = 0; x < VGA_WIDTH; x++)
        {
            terminal_putchar(x, y, ' ', 0);
        }
    }   
}

int strlen(const char* ptr)
{
    int i = 0;
    
    while(*ptr != 0)
    {
        i++;
        ptr += 1;
    }

    return i;
}

void print(const char* str)
{
    size_t len = strlen(str);

    for (int i = 0; i < len; i++)
    {
        terminal_writechar(str[i], 15);
    }
}

// \todo: remove this (for testing idt)
extern void problem();

static struct paging_4gb_chunk *kernel_chunk = 0;

void kernel_main()
{
    terminal_initialize();
    print("Terminal initialized!\n");

    kheap_init();
    idt_init();

    kernel_chunk = paging_new_4gb(PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    paging_switch(kernel_chunk);

    // test paging:
    // 1. get block of heap
    char *ptr = kzalloc(4096);
    // 2. set 0x1000 to point to physical memory ptr (0x1000 -> ptr1)
    paging_set(paging_4gb_chunk_get_directory(kernel_chunk), (void *)0x1000, ((uint32_t)ptr | PAGING_ACCESS_FROM_ALL | PAGING_IS_PRESENT | PAGING_IS_WRITEABLE));

    enable_paging();

    // 3. ptr2 points to virtual memory 0x1000 which is mapped to physical address ptr
    char *ptr2 = (char *)0x1000;
    // 4. this should also affect ptr since 0x1000 is mapped to ptr
    ptr2[0] = 'A';
    ptr2[1] = 'B';
    print(ptr2);
    print(ptr);

    enable_int();

    // testing heap
    void* ptr3 = kmalloc(500);
    void* ptr4 = kmalloc(6000);
    if (ptr3 == 0 || ptr4 == 0) {
        problem();
    }

    kfree(ptr3);
    kfree(ptr4);

    print("\nGood bye!");
}
