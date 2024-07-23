#include "tests.h"
#include "fs/file.h"
#include "string/string.h"
#include "kernel.h"
#include "disk/streamer.h"

static void test_streamer(void) {
    printk("\n\nStarting streamer tests...");

    struct disk_stream* stream = streamer_new(0);
    if (stream) {
        printk("\n[PASS] Streamer created successfully.");
    } else {
        printk("\n[FAIL] Failed to create streamer.");
    }

    streamer_seek(stream, 0x201);

    // copy old value
    unsigned char c = 0;
    streamer_read(stream, &c, 1);
    if (c == 184) {
        printk("\n[PASS] Streamer read correctly.");
    } else {
        printk("\n[FAIL] Streamer read incorrectly.");
    }

    streamer_close(stream);
    printk("\n[PASS] Streamer closed successfully.");

    printk("\nStreamer tests completed.");
}

static void test_read_initial_content(void) {
    struct file_stat stat;
    int fd = fopen("0:/test.txt", "r");
    if (fd) {
        fstat(fd, &stat);
        char buf[6];
        fread(buf, 5, 1, fd);
        buf[5] = '\0';
        if (strncmp(buf, "01234", 5) == 0) {
            printk("\n[PASS] Initial file read correctly: ");
        } else {
            printk("\n[FAIL] Initial file read incorrectly: ");
        }
        printk(buf);
        fclose(fd);
    } else {
        printk("\n[FAIL] Failed to open file for reading.");
    }
}

static void test_write_new_content(void) {
    int fd = fopen("0:/test.txt", "w");
    if (fd) {
        fwrite("98765", 1, 5, fd);
        fclose(fd);
        printk("\n[PASS] File written successfully.");
    } else {
        printk("\n[FAIL] Failed to open file for writing.");
    }

    struct file_stat stat;
    fd = fopen("0:/test.txt", "r");
    if (fd) {
        fstat(fd, &stat);
        char buf[6];
        fread(buf, 5, 1, fd);
        buf[5] = '\0';
        if (strncmp(buf, "98765", 5) == 0) {
            printk("\n[PASS] Written file read correctly: ");
        } else {
            printk("\n[FAIL] Written file read incorrectly: ");
        }
        printk(buf);
        fclose(fd);
    } else {
        printk("\n[FAIL] Failed to open file for reading.");
    }
}

static void test_append_content(void) {
    int fd = fopen("0:/test.txt", "a");
    if (fd) {
        fwrite("4", 1, 1, fd);
        fclose(fd);
        printk("\n[PASS] File appended successfully.");
    } else {
        printk("\n[FAIL] Failed to open file for appending.");
    }

    struct file_stat stat;
    fd = fopen("0:/test.txt", "r");
    if (fd) {
        fstat(fd, &stat);
        char buf[7];
        fread(buf, 6, 1, fd);
        buf[6] = '\0';
        if (strncmp(buf, "987654", 6) == 0) {
            printk("\n[PASS] Appended file read correctly: ");
        } else {
            printk("\n[FAIL] Appended file read incorrectly: ");
        }
        printk(buf);
        fclose(fd);
    } else {
        printk("\n[FAIL] Failed to open file for reading.");
    }
}

static void test_file_operations(void) {
    printk("\nStarting file operation tests...");

    test_read_initial_content();
    test_write_new_content();
    test_append_content();

    printk("\nFile operation tests completed.");
}

void tests_run(void) {
    printk("\n\nRunning tests...");
    test_file_operations();
    test_streamer();
    printk("\n\nTests completed.");
}
