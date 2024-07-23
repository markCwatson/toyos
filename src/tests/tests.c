#include "tests.h"
#include "fs/file.h"
#include "string/string.h"
#include "utils/printf.h"
#include "disk/streamer.h"
#include "memory/paging/paging.h"
#include "memory/heap/kheap.h"

extern struct paging_4gb_chunk* kernel_chunk;

static int pass_count = 0;
static int fail_count = 0;

static void test_streamer(void) {
    struct disk_stream* stream = streamer_new(0);
    if (stream) {
        pass_count += 1;
    } else {
        fail_count += 1;
    }

    streamer_seek(stream, 0x201);

    // copy old value
    unsigned char c = 0;
    streamer_read(stream, &c, 1);
    if (c == 184) {
        pass_count += 1;
    } else {
        fail_count += 1;
    }

    streamer_close(stream);
}

static void test_read_initial_content(void) {
    struct file_stat stat;
    int fd = fopen("0:/test.txt", "r");
    if (fd) {
        pass_count += 1;
        fstat(fd, &stat);
        char buf[6];
        fread(buf, 5, 1, fd);
        buf[5] = '\0';
        if (strncmp(buf, "01234", 5) == 0) {
            pass_count += 1;
        } else {
            fail_count += 1;
        }
        fclose(fd);
    } else {
        fail_count += 2;
    }
}

static void test_write_new_content(void) {
    int fd = fopen("0:/test.txt", "w");
    if (fd) {
        fwrite("98765", 1, 5, fd);
        fclose(fd);
        pass_count += 1;
    } else {
        fail_count += 1;
    }

    struct file_stat stat;
    fd = fopen("0:/test.txt", "r");
    if (fd) {
        pass_count += 1;
        fstat(fd, &stat);
        char buf[6];
        fread(buf, 5, 1, fd);
        buf[5] = '\0';
        if (strncmp(buf, "98765", 5) == 0) {
            pass_count += 1;
        } else {
            fail_count += 1;
        }
        fclose(fd);
    } else {
        fail_count += 2;
    }
}

static void test_append_content(void) {
    int fd = fopen("0:/test.txt", "a");
    if (fd) {
        fwrite("4", 1, 1, fd);
        fclose(fd);
        pass_count += 1;
    } else {
        fail_count += 1;
    }

    struct file_stat stat;
    fd = fopen("0:/test.txt", "r");
    if (fd) {
        pass_count += 1;
        fstat(fd, &stat);
        char buf[7];
        fread(buf, 6, 1, fd);
        buf[6] = '\0';
        if (strncmp(buf, "987654", 6) == 0) {
            pass_count += 1;
        } else {
            fail_count += 1;
        }
        fclose(fd);
    } else {
        fail_count += 2;
    }
}

static void test_file_operations(void) {
    test_read_initial_content();
    test_write_new_content();
    // test_append_content(); FILE_MODE_APPEND is not implemented yet
}

static void test_heap(void) {
    void* ptr = kmalloc(100);
    if (ptr) {
        pass_count += 1;
    } else {
        fail_count += 1;
    }

    kfree(ptr);
}

static void test_paging(void) {
    // 1. Get a block of heap memory
    char* ptr = kzalloc(4096);
    if (ptr) {
        pass_count += 1;
    } else {
        fail_count += 1;
    }

    // 2. Set 0x1000 to point to physical memory ptr (0x1000 -> ptr)
    int res = paging_set(paging_4gb_chunk_get_directory(kernel_chunk), (void*)0x1000, ((uint32_t)ptr | PAGING_ACCESS_FROM_ALL | PAGING_IS_PRESENT | PAGING_IS_WRITEABLE));
    if (res == 0) {
        pass_count += 1;
    } else {
        fail_count += 1;
    }

    // 3. ptr2 points to virtual memory 0x1000 which is mapped to physical address ptr
    char* ptr2 = (char*)0x1000;

    // 4. This should also affect ptr since 0x1000 is mapped to ptr
    ptr2[0] = 'A';
    ptr2[1] = 'B';

    if (ptr[0] == 'A' && ptr[1] == 'B') {
        pass_count += 1;
    } else {
        fail_count += 1;
    }

    // 5. Free the memory
    kfree(ptr);
}

void tests_run(void) {
    test_heap();
    test_paging();
    test_file_operations();
    test_streamer();

    printf("\nTests passed: %d out of %d", pass_count, pass_count + fail_count);
    printf("\nTests failed: %d\n", fail_count);
}
