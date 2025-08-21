#include <stdio.h>
#include "pattern_matching.h"

int is_ignored(const wchar_t* relForward,int isDir,Pattern* pats,int n){
    int ignore=0;
    for(int i=0;i<n;i++){
        Pattern* p=&pats[i]; if(p->dirOnly && !isDir) continue;
        int matched=0;
        if(p->anchored){ matched=match_glob(relForward,p->text,1); }
        else { if(p->dirOnly) matched=contains_dir_segment(relForward,p->text);
               else { const wchar_t* s=relForward; while(*s){ if(match_glob(s,p->text,1)){ matched=1; break; } s++; } } }
        if(matched) ignore=!p->neg;
    }
    return ignore;
}

int load_patterns(const wchar_t* root,Pattern* out){
    wchar_t fp[MAX_PATH_LEN]; swprintf(fp,MAX_PATH_LEN,L"%s\\.filterignore",root);
    FILE* f = NULL;
    errno_t err = _wfopen_s(&f, fp,L"rt, ccs=UTF-8");
    if(err != 0 || !f) err = _wfopen_s(&f, fp,L"rt");
    if(err != 0 || !f) {
        fwprintf(stderr,L"Failed to open pattern file: %s\n",fp);
        exit(1);
    }
    int count=0; wchar_t line[MAX_PATH_LEN];
    while(fgetws(line,MAX_PATH_LEN,f)){
        trim_ws(line); wchar_t* hash=wcschr(line,L'#'); if(hash){*hash=0; trim_ws(line);} if(!line[0]) continue;
        Pattern* p=&out[count]; p->neg=0; p->anchored=0; p->dirOnly=0;
        if(line[0]==L'!'){ p->neg=1; wcscpy_s(p->text,MAX_PATH_LEN,line+1);} else wcscpy_s(p->text,MAX_PATH_LEN,line);
        trim_ws(p->text);
        if(p->text[0]==L'/'){ p->anchored=1; memmove(p->text,p->text+1,(wcslen(p->text))*sizeof(wchar_t)); }
        size_t Ln=wcslen(p->text); if(Ln && p->text[Ln-1]==L'/'){ p->dirOnly=1; p->text[Ln-1]=0; }
        to_forward_slashes(p->text);
        if(p->text[0]){ count++; if(count>=MAX_PATTERNS) break; }
    }
    fclose(f); return count;
}