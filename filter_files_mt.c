// FilterFilesMT.c — Windows-compatible version with deduplication, printing full paths
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "Utils/utils.h"
#include "Utils/path_queue.h"
#include "pattern_matching.h"

#define MAX_THREADS    64

typedef struct {
    DirQueue* q;
    volatile LONG* inflight;
    volatile LONG* shutdown;
    Pattern* pats;
    int patCount;
    wchar_t root[MAX_PATH_LEN];
    int threadCount;
} ThreadArg;

/* -------- deduplication structures -------- */
typedef struct SeenNode {
    wchar_t* path;
    struct SeenNode* next;
} SeenNode;

static SeenNode* seenHead = NULL;
static CRITICAL_SECTION seenCS;

static int SeenAlready(const wchar_t* path) {
    SeenNode* node = seenHead;
    while(node) {
        if(!wcscmp(node->path, path)) return 1; // already seen
        node = node->next;
    }
    return 0;
}

static void AddToSeen(const wchar_t* path) {
    SeenNode* node = malloc(sizeof(SeenNode));
    if(!node) { fwprintf(stderr,L"alloc failed\n"); return; }
    node->path = _wcsdup(path); 
    if(!node->path) { free(node); return; }
    node->next = seenHead;
    seenHead = node;
}

/* -------- path helpers -------- */
static void path_concat(wchar_t* out,size_t outLen,const wchar_t* base,const wchar_t* name){
    size_t len=wcslen(base);
    if(len>0 && base[len-1]!=L'\\' && base[len-1]!=L'/') 
        swprintf(out,outLen,L"%s\\%s",base,name);
    else 
        swprintf(out,outLen,L"%s%s",base,name);
}

static int normalize_root(const wchar_t* in,wchar_t* out,size_t outLen){
    wchar_t abs[MAX_PATH_LEN];
    DWORD n=GetFullPathNameW(in,MAX_PATH_LEN,abs,NULL); 
    if(n==0||n>=MAX_PATH_LEN) return 0;
    size_t L=wcslen(abs); 
    while(L>0 && (abs[L-1]==L'\\'||abs[L-1]==L'/')) abs[--L]=0;
    if(L+1>=outLen) return 0; 
    abs[L]=L'\\'; abs[L+1]=0;
    wcscpy_s(out,outLen,abs); 
    return 1;
}

/* -------- enqueue helper -------- */
static __forceinline void enqueue_dir(DirQueue* q, volatile LONG* inflight, const wchar_t* dir){
    if(!dir || wcslen(dir)==0) return; // skip empty
    wchar_t* dirCopy = _wcsdup(dir);
    InterlockedIncrement(inflight);
    q_push(q, dirCopy);
}

/* -------- worker -------- */
static DWORD WINAPI worker(LPVOID param){
    ThreadArg* a=(ThreadArg*)param;
    wchar_t* dir=malloc(MAX_PATH_LEN*sizeof(wchar_t));
    wchar_t* search=malloc(MAX_PATH_LEN*sizeof(wchar_t));
    wchar_t* fullPath=malloc(MAX_PATH_LEN*sizeof(wchar_t));
    wchar_t* relBuf=malloc(MAX_PATH_LEN*sizeof(wchar_t));
    if(!dir||!search||!fullPath||!relBuf){ fwprintf(stderr,L"Heap allocation failed\n"); return 1; }

    for(;;){
        if(!q_pop(a->q, dir, a->shutdown)) break;
        wchar_t* dirPtr = _wcsdup(dir); // optional, if q_pop returns pointer

        size_t L=wcslen(dir);
        if(L && (dir[L-1]==L'\\'||dir[L-1]==L'/')) swprintf(search,MAX_PATH_LEN,L"%s*",dir);
        else swprintf(search,MAX_PATH_LEN,L"%s\\*",dir);

        WIN32_FIND_DATAW ffd; HANDLE h=FindFirstFileW(search,&ffd);
        if(h!=INVALID_HANDLE_VALUE){
            do{
                if(!wcscmp(ffd.cFileName,L".")||!wcscmp(ffd.cFileName,L"..")) continue;

                path_concat(fullPath,MAX_PATH_LEN,dir,ffd.cFileName);

                size_t rootLen=wcslen(a->root);
                const wchar_t* rel=fullPath+rootLen; 
                if(*rel==L'\\'||*rel==L'/') rel++;
                wcscpy_s(relBuf,MAX_PATH_LEN,rel); 
                to_forward_slashes(relBuf);
                if(relBuf[0]==0) continue;

                int isDir = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;
                if(is_ignored(relBuf,isDir,a->pats,a->patCount)) continue;

                if(isDir){
                    // copy fullPath for queue
                    wchar_t* dirCopy = _wcsdup(fullPath);
                    enqueue_dir(a->q,a->inflight,dirCopy);
                } else { 
                    EnterCriticalSection(&seenCS);
                    if(!SeenAlready(relBuf)) {
                        AddToSeen(relBuf);
                        // copy fullPath for printing
                        wchar_t printBuf[MAX_PATH_LEN];
                        wcscpy_s(printBuf, MAX_PATH_LEN, fullPath);
                        if(wcscmp(dir, L"")==0) { LeaveCriticalSection(&seenCS);continue; } // skip empty relative paths
                        
                        /*struct stat fileStat;
                        if(stat(wchar_to_utf8(fullPath), &fileStat) != 0) {
                            fwprintf(stderr,L"Unable to get file stats for: %s\n", fullPath);
                            return 1;
                        }

                        wprintf(L"%s|%lld\n", printBuf, (long long)fileStat.st_size);*/
                        wprintf(L"%s\n", printBuf);
                    }
                    LeaveCriticalSection(&seenCS);
                }

            }while(FindNextFileW(h,&ffd));
            FindClose(h);
        }

        free(dirPtr);

        if(InterlockedDecrement(a->inflight)==0){
            *a->shutdown=1;
            for(int i=0;i<a->threadCount;i++) ReleaseSemaphore(a->q->itemsSem,1,NULL);
        }
    }

    free(dir); free(search); free(fullPath); free(relBuf);
    return 0;
}

/* -------- main -------- */
int wmain(int argc,wchar_t* argv[]){
    if(argc<2){ fwprintf(stderr,L"Usage: %s <root> [threads]\n",argv[0]); return 2; }

    wchar_t root[MAX_PATH_LEN];
    if(!normalize_root(argv[1],root,MAX_PATH_LEN)){ fwprintf(stderr,L"Failed to resolve root: %s\n",argv[1]); return 3; }
    DWORD attr=GetFileAttributesW(root);
    if(attr==INVALID_FILE_ATTRIBUTES || !(attr&FILE_ATTRIBUTE_DIRECTORY)){ fwprintf(stderr,L"Path is not a directory: %s\n",root); return 3; }

    int threads=1; 
    if(argc>=3){ 
        threads=_wtoi(argv[2]); 
        if(threads<1) threads=1; 
        if(threads>MAX_THREADS) threads=MAX_THREADS; 
    }

    Pattern* pats = malloc(sizeof(Pattern)*MAX_PATTERNS); 
    if(!pats){ fwprintf(stderr,L"alloc patterns failed\n"); return 1; }
    int patCount = load_patterns(root, pats);

    DirQueue q={0};
    if(!q_init(&q)){ fwprintf(stderr,L"Queue init failed\n"); free(pats); q_destroy(&q); return 1; }

    volatile LONG inflight=0, shutdown=0;
    enqueue_dir(&q,&inflight,root);

    ThreadArg a={0};
    a.q=&q; a.inflight=&inflight; a.shutdown=&shutdown;
    a.pats=pats; a.patCount=patCount; wcscpy_s(a.root,MAX_PATH_LEN,root);
    a.threadCount=threads;

    InitializeCriticalSection(&seenCS);  // init dedup CS

    HANDLE th[MAX_THREADS]={0};
    for(int i=0;i<threads;i++){
        th[i]=CreateThread(NULL,0,worker,&a,0,NULL);
        if(!th[i]){ fwprintf(stderr,L"CreateThread failed\n"); shutdown=1; threads=i; break; }
    }

    WaitForMultipleObjects(threads,th,TRUE,INFINITE);
    for(int i=0;i<threads;i++) if(th[i]) CloseHandle(th[i]);

    // free deduplication list
    EnterCriticalSection(&seenCS);
    SeenNode* node = seenHead;
    while(node){
        SeenNode* next = node->next;
        free(node->path);
        free(node);
        node = next;
    }
    LeaveCriticalSection(&seenCS);
    DeleteCriticalSection(&seenCS);

    q_destroy(&q);
    free(pats);

    return 0;
}