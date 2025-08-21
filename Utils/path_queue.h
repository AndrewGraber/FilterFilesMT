#ifndef PATH_QUEUE_H
#define PATH_QUEUE_H

#include <windows.h>

#define MAX_PATH_LEN   512
#define QUEUE_CAP      8192

typedef struct {
    wchar_t (*items)[MAX_PATH_LEN];  // pointer to heap array
    LONG head;
    LONG tail;
    CRITICAL_SECTION cs;
    HANDLE slotsSem;    // counts free slots
    HANDLE itemsSem;    // counts queued items
} DirQueue;

int q_init(DirQueue* q);
void q_destroy(DirQueue* q);
int q_push(DirQueue* q,const wchar_t* path);
int q_pop(DirQueue* q,wchar_t* out,volatile LONG* shutdown);

#endif