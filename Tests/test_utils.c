#include <wchar.h>
#include <stdio.h>
#include <string.h>
#include "../Utils/utils.h"
#include "../Utils/path_queue.h"

int test_trim_ws(void) {
    wprintf(L"=== Tests for trim_ws ===\n");

    struct { wchar_t input[128]; wchar_t expected[128]; } tests[] = {
        {L"   hello world   ", L"hello world"},
        {L"\t\tfoo bar\t", L"foo bar"},
        {L"\r\n  baz \t\r\n", L"baz"},
        {L"nospaces", L"nospaces"},
        {L"    ", L""},
        {L"", L""},
    };

    int failed = 0;
    int total = sizeof(tests) / sizeof(*tests);

    for(int i=0; i<total; i++){
        wchar_t buffer[128];
        wcscpy_s(buffer, 128, tests[i].input);
        trim_ws(buffer);
        if(wcscmp(buffer, tests[i].expected) != 0) {
            wprintf(L"[FAIL] Case %d: input='%s', expected='%s', got='%s'\n", i, tests[i].input, tests[i].expected, buffer);
            failed++;
        } else {
            wprintf(L"[PASS] Case %d\n", i);
        }
    }

    wprintf(L"%d/%d test cases passed.\n", (total-failed), total);
    return failed;
}

int test_to_forward_slashes(void) {
    wprintf(L"=== Tests for to_forward_slashes ===\n");

    struct { wchar_t input[128]; wchar_t expected[128]; } tests[] = {
        {L"C:\\Users\\Test", L"C:/Users/Test"},
        {L"no/slashes", L"no/slashes"},
        {L"\\leading\\", L"/leading/"},
        {L"", L""},
    };

    int failed = 0;
    int total = sizeof(tests)/sizeof(*tests);

    for(int i=0; i<total; i++){
        wchar_t buffer[128];
        wcscpy_s(buffer, 128, tests[i].input);
        to_forward_slashes(buffer);
        if(wcscmp(buffer, tests[i].expected) != 0){
            wprintf(L"[FAIL] Test %d: input='%s', expected='%s', got='%s'\n",
                    i, tests[i].input, tests[i].expected, buffer);
            failed++;
        } else {
            wprintf(L"[PASS] Test %d\n", i);
        }
    }

    wprintf(L"%d/%d test cases passed.\n", (total-failed), total);
    return failed;
}

int test_ieq(void) {
    wprintf(L"=== Tests for ieq ===\n");

    struct { wchar_t a; wchar_t b; int expected; } tests[] = {
        {L'a', L'A', 1},
        {L'Z', L'z', 1},
        {L'a', L'b', 0},
        {L'k', L'k', 1},
        {L'G', L'H', 0},
    };

    int failed = 0;
    int total = sizeof(tests)/sizeof(*tests);

    for(int i=0; i<total; i++){
        int result = ieq(tests[i].a, tests[i].b);
        if(result != tests[i].expected){
            wprintf(L"[FAIL] Test %d: ieq('%c','%c') expected %d, got %d\n",
                    i, tests[i].a, tests[i].b, tests[i].expected, result);
            failed++;
        } else {
            wprintf(L"[PASS] Test %d\n", i);
        }
    }

    wprintf(L"%d/%d test cases passed.\n", (total-failed), total);
    return failed;
}

int test_match_glob(void) {
    wprintf(L"=== Tests for match_glob ===\n");

    struct { 
        const wchar_t* str; 
        const wchar_t* pat; 
        int allowSlashCross; 
        int expected; 
    } tests[] = {
        {L"foo.txt", L"foo.txt", 0, 1},
        {L"foo.txt", L"*.txt", 0, 1},
        {L"foo.txt", L"*.log", 0, 0},
        {L"dir/file.txt", L"dir/*.txt", 0, 1},
        {L"dir/file.txt", L"*.txt", 0, 0},
        {L"dir/file.txt", L"*.txt", 1, 1},
        {L"dir/sub/file.txt", L"dir/**/file.txt", 0, 1},
        {L"dir/sub/file.txt", L"dir/*/file.txt", 0, 1},
        {L"dir/a/b/c.txt", L"dir/**.txt", 0, 1},
        {L"foo", L"*", 0, 1},
        {L"", L"*", 0, 0},
        {L"", L"", 0, 1},
        {L"abc", L"", 0, 0},
    };

    int failed = 0;
    int total = sizeof(tests) / sizeof(*tests);

    for (int i=0; i<total; i++) {
        int got = match_glob(tests[i].str, tests[i].pat, tests[i].allowSlashCross);
        if (got != tests[i].expected) {
            wprintf(L"[FAIL] Case %d: str='%s', pat='%s', allowSlashCross=%d, expected=%d, got=%d\n", 
                    i, tests[i].str, tests[i].pat, tests[i].allowSlashCross, tests[i].expected, got);
            failed++;
        } else {
            wprintf(L"[PASS] Case %d\n", i);
        }
    }

    wprintf(L"%d/%d test cases passed.\n", (total-failed), total);
    return failed;
}

int test_contains_dir_segment(void) {
    wprintf(L"=== Tests for contains_dir_segment ===\n");

    struct { 
        const wchar_t* rel; 
        const wchar_t* name; 
        int expected; 
    } tests[] = {
        {L"foo/bar/baz", L"bar", 1},
        {L"foo/bar/baz", L"baz", 1},
        {L"foo/bar/baz", L"foo", 1},
        {L"foo/bar/baz", L"qux", 0},
        {L"foo/bar/baz", L"ba", 0},
        {L"single", L"single", 1},
        {L"single", L"other", 0},
        {L"", L"anything", 0},
    };

    int failed = 0;
    int total = sizeof(tests) / sizeof(*tests);

    for (int i=0; i<total; i++) {
        int got = contains_dir_segment(tests[i].rel, tests[i].name);
        if (got != tests[i].expected) {
            wprintf(L"[FAIL] Case %d: rel='%s', name='%s', expected=%d, got=%d\n",
                    i, tests[i].rel, tests[i].name, tests[i].expected, got);
            failed++;
        } else {
            wprintf(L"[PASS] Case %d\n", i);
        }
    }

    wprintf(L"%d/%d test cases passed.\n", (total-failed), total);
    return failed;
}