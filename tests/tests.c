#include "tests.h"
#include "fs/file.h"
#include "string/string.h"
#include "utils/printf.h"
#include "disk/streamer.h"
#include "memory/paging/paging.h"
#include "memory/heap/kheap.h"
#include "kernel.h"

extern struct paging_4gb_chunk* kernel_chunk;

#define MAX_TESTS 100

typedef struct {
    int test_num;
    const char* description;
    bool passed;
} test_result_t;

static test_result_t test_results[MAX_TESTS];
static int test_count = 0;
static int pass_count = 0;
static int fail_count = 0;

static inline void register_test(int test_num, const char* description, bool condition) {
    if (test_count < MAX_TESTS) {
        test_results[test_count].test_num = test_num;
        test_results[test_count].description = description;
        test_results[test_count].passed = condition;
        if (condition) {
            pass_count++;
        } else {
            fail_count++;
        }
        test_count++;
    } else {
        panick("Too many tests registered");
    }
}

static void print_test_summary(void) {
    printf("\n\nTest Summary:\n");
    printf("Total tests run: %d\n", test_count);
    printf("Tests passed: %d\n", pass_count);
    printf("Tests failed: %d\n", fail_count);
    if (fail_count > 0) {
        printf("\nFailed Tests:\n");
        for (int i = 0; i < test_count; i++) {
            if (!test_results[i].passed) {
                printf("Test %d: %s\n", test_results[i].test_num, test_results[i].description);
            }
        }
    }
}

static void test_streamer(void) {
    int test_num = 1;
    struct disk_stream* stream = streamer_new(0);
    register_test(test_num++, "Streamer creation", stream != NULL);

    if (stream) {
        streamer_seek(stream, 0x201);

        unsigned char c = 0;
        streamer_read(stream, &c, 1);
        register_test(test_num++, "Streamer read", c == 184);

        streamer_close(stream);
        register_test(test_num++, "Streamer close", true);
    }
}

static void test_read_initial_content(void) {
    int test_num = 1;
    struct file_stat stat;
    int fd = fopen("0:/test.txt", "r");
    register_test(test_num++, "Open file for reading", fd != 0);

    if (fd) {
        fstat(fd, &stat);
        char buf[6];
        fread(buf, 5, 1, fd);
        buf[5] = '\0';
        register_test(test_num++, "Read initial content", strncmp(buf, "01234", 5) == 0);
        fclose(fd);
    }
}

static void test_write_new_content(void) {
    int test_num = 1;
    int fd = fopen("0:/test.txt", "w");
    register_test(test_num++, "Open file for writing", fd != 0);

    if (fd) {
        fwrite("98765", 1, 5, fd);
        fclose(fd);
        register_test(test_num++, "Write new content", true);
    }

    fd = fopen("0:/test.txt", "r");
    register_test(test_num++, "Open file for reading new content", fd != 0);

    if (fd) {
        struct file_stat stat;
        fstat(fd, &stat);
        char buf[6];
        fread(buf, 5, 1, fd);
        buf[5] = '\0';
        register_test(test_num++, "Read new content", strncmp(buf, "98765", 5) == 0);
        fclose(fd);
    }
}

static void test_append_content(void) {
    int test_num = 1;
    int fd = fopen("0:/test.txt", "a");
    register_test(test_num++, "Open file for appending", fd != 0);

    if (fd) {
        fwrite("4", 1, 1, fd);
        fclose(fd);
        register_test(test_num++, "Append content", true);
    }

    fd = fopen("0:/test.txt", "r");
    register_test(test_num++, "Open file for reading appended content", fd != 0);

    if (fd) {
        struct file_stat stat;
        fstat(fd, &stat);
        char buf[7];
        fread(buf, 6, 1, fd);
        buf[6] = '\0';
        register_test(test_num++, "Read appended content", strncmp(buf, "987654", 6) == 0);
        fclose(fd);
    }
}

static void test_file_operations(void) {
    test_read_initial_content();
    test_write_new_content();
    // test_append_content(); FILE_MODE_APPEND is not implemented yet
}

static void test_heap(void) {
    int test_num = 1;
    void* ptr = kmalloc(100);
    register_test(test_num++, "Heap allocation", ptr != NULL);

    kfree(ptr);
    register_test(test_num++, "Heap free", true);
}

static void test_paging(void) {
    int test_num = 1;

    char* ptr = kzalloc(4096);
    register_test(test_num++, "Heap block allocation", ptr != NULL);

    int res = paging_set(paging_4gb_chunk_get_directory(kernel_chunk), (void*)0x1000, ((uint32_t)ptr | PAGING_ACCESS_FROM_ALL | PAGING_IS_PRESENT | PAGING_IS_WRITEABLE));
    register_test(test_num++, "Paging set", res == 0);

    char* ptr2 = (char*)0x1000;

    ptr2[0] = 'A';
    ptr2[1] = 'B';

    register_test(test_num++, "Paging write to virtual address", ptr[0] == 'A' && ptr[1] == 'B');
    register_test(test_num++, "Paging read from virtual address", ptr2[0] == 'A' && ptr2[1] == 'B');

    kfree(ptr);
    register_test(test_num++, "Free memory", true);
}

void tests_run(void) {
    test_heap();
    test_paging();
    test_file_operations();
    test_streamer();

    print_test_summary();
}
