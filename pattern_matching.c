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

int load_patterns_from_file(const wchar_t* fp, Pattern* out){
    FILE* f = NULL;
    errno_t err = _wfopen_s(&f, fp,L"rt, ccs=UTF-8");
    if(err != 0 || !f) err = _wfopen_s(&f, fp,L"rt");
    if(err != 0 || !f) {
        fwprintf(stderr,L"Pattern file not found: %s\n",fp);
        return 0;
    }
    int count=0; wchar_t line[MAX_PATH_LEN];
    while(fgetws(line,MAX_PATH_LEN,f)){
        int result = parse_pattern(line, &out[count]);

        if(result){
            count++;
            if(count >= MAX_PATTERNS) break;
        }
    }
    fclose(f); return count;
}

int parse_pattern(wchar_t* patIn, Pattern* patOut) {
    trim_ws(patIn);
    wchar_t* hash=wcschr(patIn,L'#');
    if(hash) {
        *hash=0;
        trim_ws(patIn);
    }

    if(!patIn[0]) return 0;

    patOut->neg=0; patOut->anchored=0; patOut->dirOnly=0;

    if(patIn[0]==L'!') {
        patOut->neg=1;
        wcscpy_s(patOut->text,MAX_PATH_LEN,patIn+1);
    } else {
        wcscpy_s(patOut->text,MAX_PATH_LEN,patIn);
    }

    trim_ws(patOut->text);

    if(patOut->text[0]==L'/'){
        patOut->anchored=1;
        memmove(patOut->text, patOut->text+1, (wcslen(patOut->text))*sizeof(wchar_t));
    }

    size_t Ln=wcslen(patOut->text);
    if(Ln && patOut->text[Ln-1]==L'/'){
        patOut->dirOnly=1;
        patOut->text[Ln-1]=0;
    }

    to_forward_slashes(patOut->text);
    return 1;
}