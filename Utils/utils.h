#ifndef UTILS_H
#define UTILS_H

#include <wchar.h>

#define DEFAULT_PATH L"./"
#define DEFAULT_THREADS 4
#define MAX_THREADS 16
#define DEFAULT_PATTERN L"*"
#define DEFAULT_PATTERN_FILE L"./.filterignore"

typedef struct {
    int numPatArgs, numThreads;
    wchar_t* path;
    wchar_t* patternFile;
    wchar_t** patArgs;
} CmdArgs;

void print_usage(wchar_t *prog_name);
void args_destroy(CmdArgs* args);
void parse_args(int argc, wchar_t* argv[], CmdArgs* out);
void trim_ws(wchar_t* s);
void to_forward_slashes(wchar_t* s);
int ieq(wchar_t a, wchar_t b);
char* wchar_to_utf8(const wchar_t* wstr);

int match_glob(const wchar_t* str,const wchar_t* pat,int allowSlashCross);
int contains_dir_segment(const wchar_t* rel,const wchar_t* name);

#endif