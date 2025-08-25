#include "utils.h"
#include <wchar.h>
#include <windows.h>
#include <string.h>

void print_usage(wchar_t* prog_name) {
    fwprintf(stderr,L"Usage: %s --path <path> --threads <n>\n",prog_name);
    exit(2);
}

void args_destroy(CmdArgs* args) {
    if (!args) return;

    free(args->patArgs);
    free(args);
}

void parse_args(int argc, wchar_t* argv[], CmdArgs* out) {
    out->numPatArgs = 0;
    out->patArgs = NULL;
    out->path = DEFAULT_PATH;
    out->numThreads = DEFAULT_THREADS;
    out->patternFile = DEFAULT_PATTERN_FILE;

    int curArg = 1;
    while (curArg < argc) {
        if(wcscmp(argv[curArg], L"--path") == 0) {
            if (++curArg < argc) {
                out->path = argv[curArg];
            } else {
                fwprintf(stderr, L"Expected a string after --path\n");
                print_usage(argv[0]);
            }
        } else if (wcscmp(argv[curArg], L"--pattern-file") == 0) {
            if (++curArg < argc) {
                out->patternFile = argv[curArg];
            } else {
                fwprintf(stderr, L"Expected a file path after --pattern-file\n");
                print_usage(argv[0]);
            }
        } else if(wcscmp(argv[curArg], L"--threads") == 0) {
            if (++curArg < argc) {
                out->numThreads = _wtoi(argv[curArg]);
                if(out->numThreads > MAX_THREADS) out->numThreads = MAX_THREADS;
            } else {
                fwprintf(stderr, L"Expected a number after --threads\n");
                print_usage(argv[0]);
            }
        } else if(wcscmp(argv[curArg], L"--pattern") == 0) {
            if (++curArg < argc) {
                out->numPatArgs++;
                out->patArgs = realloc(out->patArgs, sizeof(wchar_t*) * out->numPatArgs);
                if (!out->patArgs) {
                    fwprintf(stderr, L"Memory allocation failed for pattern arguments\n");
                    exit(1);
                }
                out->patArgs[out->numPatArgs - 1] = argv[curArg];
            } else {
                fwprintf(stderr, L"Expected a glob string after --pattern\n");
                print_usage(argv[0]);
            }
        } else {
            fwprintf(stderr, L"Argument not recognized: %s\n", argv[curArg]);
            print_usage(argv[0]);
        }
        curArg++;
    }
}

void trim_ws(wchar_t* s) {
    if (!s) return;

    // Trim leading whitespace
    wchar_t* start = s;
    while (*start == L' ' || *start == L'\t' || *start == L'\r' || *start == L'\n') start++;

    // Shift string back to the start
    if (start != s) {
        size_t len = wcslen(start);
        memmove(s, start, (len + 1) * sizeof(wchar_t)); // +1 for null terminator
    }

    // Trim trailing whitespace
    size_t n = wcslen(s);
    while (n > 0) {
        wchar_t c = s[n - 1];
        if (c != L' ' && c != L'\t' && c != L'\r' && c != L'\n') break;
        s[--n] = 0;
    }
}

int is_absolute_path(const wchar_t* path) {
    if (!path || !*path) return 0;

    // Check for absolute path (Windows)
    if (wcslen(path) > 2 && path[1] == L':' && (path[2] == L'\\' || path[2] == L'/')) {
        return 1;
    }

    // Check for UNC path
    if (wcslen(path) > 1 && path[0] == L'\\' && path[1] == L'\\') {
        return 1;
    }

    return 0;
}

void to_forward_slashes(wchar_t* s){
    for(; *s; ++s) if(*s==L'\\') *s=L'/';
}

int prepend_folder_inplace(wchar_t *buffer, size_t buffer_capacity, const wchar_t *folder) {
    if (!buffer || !folder) return 0;

    size_t folder_len = wcslen(folder);
    size_t file_len   = wcslen(buffer);
    fwprintf(stderr, L"Folder len: %zu, file len: %zu\n", folder_len, file_len);

    int need_backslash = (folder_len > 0 &&
        folder[folder_len - 1] != L'\\' &&
        folder[folder_len - 1] != L'/');

    size_t total_len = folder_len + need_backslash + file_len + 1; // +1 for null terminator
    if (total_len > buffer_capacity) return 0; // fail safely

    fwprintf(stderr, L"Before shifting filename\n");

    // Shift filename backwards safely (from end, include null terminator)
    for (int i = (int)file_len; i >= 0; i--) {
        buffer[folder_len + need_backslash + i] = buffer[i];
    }

    fwprintf(stderr, L"After shifting filename\n");

    // Copy folder into start of buffer
    for (size_t i = 0; i < folder_len; i++) {
        buffer[i] = folder[i];
    }

    fwprintf(stderr, L"After copying folder\n");

    // Add '\' if needed
    if (need_backslash) {
        buffer[folder_len] = L'\\';
    }

    return 1; // success
}

int ieq(wchar_t a, wchar_t b){
    if(a>=L'A' && a<=L'Z') a+=32;
    if(b>=L'A' && b<=L'Z') b+=32;
    return a==b;
}

char* wchar_to_utf8(const wchar_t* wstr) {
    if (!wstr) return NULL;

    // Determine required buffer size
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (size <= 0) return NULL;

    // Allocate buffer
    char* buffer = (char*)malloc(size);
    if (!buffer) return NULL;

    // Perform conversion
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, buffer, size, NULL, NULL);
    return buffer;
}

/* -------- glob matching -------- */
int match_glob(const wchar_t* str, const wchar_t* pat, int allowSlashCross) {
    while (*pat) {
        if (*pat == L'*') {
            int dbl = (pat[1] == L'*');
            if (dbl) pat++;      // skip extra '*'
            pat++;                // skip current '*'

            if (!*pat) return dbl || *str != 0; // trailing '*' matches; single '*' requires at least one char

            if (dbl) {
                // '**' can match zero or more chars, including '/'
                for (const wchar_t* s = str; ; ++s) {
                    if (match_glob(s, pat, 1)) return 1;
                    if (!*s) break;
                }
                return 0;
            } else {
                // single '*' must match at least one character, optionally stopping at '/'
                if (!*str) return 0; // nothing to match
                for (const wchar_t* s = str; *s && (allowSlashCross || *s != L'/'); ++s) {
                    if (match_glob(s + 1, pat, allowSlashCross)) return 1;
                }
                return 0;
            }
        } else {
            if (!*str) return 0;
            if (!ieq(*str, *pat)) return 0;
            str++; pat++;
        }
    }
    return *str == 0;
}

int contains_dir_segment(const wchar_t* rel, const wchar_t* name) {
    size_t n = wcslen(name);
    const wchar_t* p = rel;
    while (*p) {
        const wchar_t* s = p;
        while (*p && *p != L'/') p++;
        size_t m = (size_t)(p - s);
        if (m == n) {
            size_t i = 0;
            for (; i < m; i++) if (!ieq(s[i], name[i])) break;
            if (i == m) return 1;
        }
        if(*p==L'/') p++;
    } return 0;
}