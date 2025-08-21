#include "utils.h"
#include <wchar.h>
#include <windows.h>
#include <string.h>

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

void to_forward_slashes(wchar_t* s){
    for(; *s; ++s) if(*s==L'\\') *s=L'/';
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