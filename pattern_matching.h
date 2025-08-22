#ifndef PATTERN_MATCHING_H
#define PATTERN_MATCHING_H

#include <wchar.h>
#include "Utils/path_queue.h"
#include "Utils/utils.h"

#define MAX_PATTERNS 1024

typedef struct {
    wchar_t text[MAX_PATH_LEN];
    int neg, anchored, dirOnly;
} Pattern;

int is_ignored(const wchar_t* relForward,int isDir,Pattern* pats,int n);
int load_patterns(const wchar_t* root,Pattern* out);
int parse_pattern(wchar_t* patIn, Pattern* patOut);

#endif // PATTERN_MATCHING_H