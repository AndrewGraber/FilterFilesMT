#ifndef UTILS_H
#define UTILS_H

#include <wchar.h>

void trim_ws(wchar_t* s);
void to_forward_slashes(wchar_t* s);
int ieq(wchar_t a, wchar_t b);
char* wchar_to_utf8(const wchar_t* wstr);

int match_glob(const wchar_t* str,const wchar_t* pat,int allowSlashCross);
int contains_dir_segment(const wchar_t* rel,const wchar_t* name);

#endif