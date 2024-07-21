#ifndef KERNEL_H
#define KERNEL_H

#define TOYOS_MAX_PATH  108

#define ERROR(value)    ((void*)(value))
#define ISERROR(value)  (((int)(value)) < 0)
#define ERROR_I(value)  ((int)(value))

void kernel_main(void);
void printk(const char* str);

#endif
