#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

/* Forward declare the vulnerable strcpy from the production code */
extern char* strcpy(char* dest, const char* src);

static jmp_buf jump_buffer;
static int segfault_caught = 0;

void segfault_handler(int sig) {
    segfault_caught = 1;
    longjmp(jump_buffer, 1);
}

START_TEST(test_strcpy_buffer_overflow_protection)
{
    /* Invariant: strcpy must not read/write beyond declared buffer bounds */
    const char *payloads[] = {
        "valid_short_string",                    /* Valid input within bounds */
        "A",                                      /* Boundary: minimal string */
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"      /* Exploit: 34 chars > 16-byte buffer */
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        char dest[16];
        memset(dest, 0, sizeof(dest));
        
        signal(SIGSEGV, segfault_handler);
        segfault_caught = 0;
        
        if (setjmp(jump_buffer) == 0) {
            strcpy(dest, payloads[i]);
        }
        
        signal(SIGSEGV, SIG_DFL);
        
        /* Assert: no segfault occurred (buffer overflow detected) */
        ck_assert_int_eq(segfault_caught, 0);
        
        /* Assert: destination is null-terminated */
        ck_assert(dest[sizeof(dest) - 1] == 0 || strlen(dest) < sizeof(dest));
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_strcpy_buffer_overflow_protection);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}