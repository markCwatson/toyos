#include "tests.h"
#include "fs/file.h"
#include "string/string.h"
#include "stdlib/printf.h"
#include "disk/streamer.h"
#include "memory/paging/paging.h"
#include "memory/heap/kheap.h"
#include "kernel.h"
#include "task/process.h"
#include "keyboard/keyboard.h"

extern struct paging_4gb_chunk* kernel_chunk;

#define MAX_TESTS 100

// Structure to hold the result of each test
typedef struct {
    int test_num;            /**< The test number. */
    const char* description; /**< A brief description of the test. */
    bool passed;             /**< Whether the test passed or failed. */
} test_result_t;

static test_result_t test_results[MAX_TESTS];
static int test_count = 0;
static int pass_count = 0;
static int fail_count = 0;

/**
 * @brief Registers the result of a test.
 *
 * This function stores the result of a test, including its number, description,
 * and whether it passed or failed. It also updates the count of passed and failed tests.
 *
 * @param description A description of the test.
 * @param condition Whether the test passed (true) or failed (false).
 */
static inline void register_test(const char* description, bool condition) {
    if (test_count < MAX_TESTS) {
        test_results[test_count].test_num = test_count;
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

/**
 * @brief Prints a summary of the test results.
 *
 * This function outputs the total number of tests run, the number of tests passed,
 * and the number of tests failed. It also lists the details of any failed tests.
 */
static void print_test_summary(void) {
    printf("\n\nTest Summary:\n");
    printf("Total tests run: %d\n", test_count);
    printf_colored("Tests passed: %d\n", COLOR_GREEN, COLOR_BLACK, pass_count);
    printf_colored("Tests failed: %d\n", COLOR_RED, COLOR_BLACK, fail_count);
    if (fail_count > 0) {
        printf_colored("\nFailed Tests:\n", COLOR_RED, COLOR_BLACK);
        for (int i = 0; i < test_count; i++) {
            if (!test_results[i].passed) {
                printf_colored("Test %d: %s\n", COLOR_RED, COLOR_BLACK, test_results[i].test_num, test_results[i].description);
            }
        }
    }
}

/**
 * @brief Tests the keyboard functionality.
 */
static void test_keyboard(void) {
    keyboard_push('A');
    struct process* proc = process_current();
    register_test("Keyboard push A", proc->keyboard.buffer[0] == 'A');

    keyboard_push('B');
    register_test("Keyboard push B", proc->keyboard.buffer[1] == 'B');

    keyboard_pop();
    register_test("Keyboard pop A", proc->keyboard.buffer[0] == 0);
}

/**
 * @brief Tests loading and switching to a user program.
 */
static void test_user_program(void) {
    struct process* cur_process = process_current();
    register_test("Current process", cur_process != NULL);

    struct process* process = NULL;
    int res = process_load_switch("0:/test.bin", &process);
    register_test("Load user program", res == 0);

    // switch back
    res = process_load_switch(cur_process->filename, &cur_process);
    register_test("Switch back to kernel", res == 0);
}

/**
 * @brief Tests the disk streamer functionality.
 *
 * This function tests creating a disk stream, reading from it, and closing it.
 */
static void test_streamer(void) {
    struct disk_stream* stream = streamer_new(0);
    register_test("Streamer creation", stream != NULL);

    if (stream) {
        streamer_seek(stream, 0x201);

        unsigned char c = 0;
        streamer_read(stream, &c, 1);
        register_test("Streamer read", c == 184);

        streamer_close(stream);
        register_test("Streamer close", true);
    }
}

/**
 * @brief Tests reading the initial content of a file.
 *
 * This function tests opening a file, reading its content, and verifying the read data.
 */
static void test_read_initial_content(void) {
    struct file_stat stat;
    int fd = fopen("0:/test.txt", "r");
    register_test("Open file for reading", fd != 0);

    if (fd) {
        fstat(fd, &stat);
        char buf[6];
        fread(buf, 5, 1, fd);
        buf[5] = '\0';
        register_test("Read initial content", strncmp(buf, "01234", 5) == 0);
        fclose(fd);
    }
}

/**
 * @brief Tests writing new content to a file.
 *
 * This function tests opening a file for writing, writing new content, and then verifying the content.
 */
static void test_write_new_content(void) {
    int fd = fopen("0:/test.txt", "w");
    register_test("Open file for writing", fd != 0);

    if (fd) {
        fwrite("98765", 1, 5, fd);
        fclose(fd);
        register_test("Write new content", true);
    }

    fd = fopen("0:/test.txt", "r");
    register_test("Open file for reading new content", fd != 0);

    if (fd) {
        struct file_stat stat;
        fstat(fd, &stat);
        char buf[6];
        fread(buf, 5, 1, fd);
        buf[5] = '\0';
        register_test("Read new content", strncmp(buf, "98765", 5) == 0);
        fclose(fd);
    }
}

/**
 * @brief Tests appending content to a file.
 *
 * This function tests opening a file for appending, writing additional data, and then verifying the appended content.
 */
static void test_append_content(void) {
    int fd = fopen("0:/test.txt", "a");
    register_test("Open file for appending", fd != 0);

    if (fd) {
        fwrite("4", 1, 1, fd);
        fclose(fd);
        register_test("Append content", true);
    }

    fd = fopen("0:/test.txt", "r");
    register_test("Open file for reading appended content", fd != 0);

    if (fd) {
        struct file_stat stat;
        fstat(fd, &stat);
        char buf[7];
        fread(buf, 6, 1, fd);
        buf[6] = '\0';
        register_test("Read appended content", strncmp(buf, "987654", 6) == 0);
        fclose(fd);
    }
}

/**
 * @brief Runs a series of file operation tests.
 *
 * This function includes tests for reading initial content, writing new content,
 * and appending content to files.
 */
static void test_file_operations(void) {
    test_read_initial_content();
    test_write_new_content();
    test_append_content(); // failing because FILE_MODE_APPEND is not implemented yet
}

/**
 * @brief Tests the kernel heap functionality.
 *
 * This function tests memory allocation and deallocation using the kernel heap.
 */
static void test_heap(void) {
    void* ptr = kmalloc(100);
    register_test("Heap allocation", ptr != NULL);

    kfree(ptr);
    register_test("Heap free", true);
}

/**
 * @brief Tests the paging system functionality.
 *
 * This function includes tests for allocating a heap block, setting paging,
 * and verifying read/write operations to a mapped virtual address.
 */
static void test_paging(void) {
    char* ptr = kzalloc(4096);
    register_test("Heap block allocation", ptr != NULL);

    int res = paging_set(paging_4gb_chunk_get_directory(kernel_chunk), (void*)0x1000, ((uint32_t)ptr | PAGING_ACCESS_FROM_ALL | PAGING_IS_PRESENT | PAGING_IS_WRITEABLE));
    register_test("Paging set", res == 0);

    char* ptr2 = (char*)0x1000;

    ptr2[0] = 'A';
    ptr2[1] = 'B';

    register_test("Paging write to virtual address", ptr[0] == 'A' && ptr[1] == 'B');
    register_test("Paging read from virtual address", ptr2[0] == 'A' && ptr2[1] == 'B');

    kfree(ptr);
    register_test("Free memory", true);
}

/**
 * @brief Main function to run all tests.
 *
 * This function orchestrates the running of all test cases, including heap,
 * paging, file operations, and disk streamer functionality. It also prints a summary of the results.
 */
void tests_run(void) {
    test_heap();
    test_paging();
    test_file_operations();
    test_streamer();
    test_keyboard();
    test_user_program();

    print_test_summary();
}
