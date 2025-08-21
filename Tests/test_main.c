// Entry point for tests for FilterFilesMT

// COMPILE WITH: "cl /W4 /DUNICODE /D_UNICODE Utils\*.c Tests\*.c /Fe:TestFilterFilesMT.exe"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_utils.h"
#include "test_queue.h"

typedef int (*TestFunc)(void);

typedef struct {
    const char* name;
    TestFunc func;
} TestEntry;

TestEntry tests[] = {
    {"trim_ws", test_trim_ws},
    {"to_forward_slashes", test_to_forward_slashes},
    {"ieq", test_ieq},
    {"match_glob", test_match_glob},
    {"contains_dir_segment", test_contains_dir_segment},
    {"queue_st", test_queue_st},
    {"queue_mt", test_queue_mt}
};

int main(int argc, char** argv) {
    int total_failures = 0;
    int n_tests = sizeof(tests)/sizeof(tests[0]);

    if (argc == 2) {
        // Run a single test by name
        for (int i=0; i<n_tests; i++) {
            if (strcmp(argv[1], tests[i].name) == 0) {
                int fails = tests[i].func();
                return fails;
            }
        }
        printf("Test '%s' not found.\n", argv[1]);
        return 1;
    } else {
        // Run all tests
        for (int i=0; i<n_tests; i++) {
            total_failures += tests[i].func();
        }
    }

    if (total_failures == 0) {
        printf("\nAll tests passed!\n");
    } else {
        printf("\n%d test case(s) failed.\n", total_failures);
    }

    return total_failures;
}