#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>

#include "path_queue.h"

int q_init(DirQueue* q) {
    q->items = malloc(sizeof(wchar_t[QUEUE_CAP][MAX_PATH_LEN]));
    if (!q->items) return 0;
    q->head = q->tail = 0;
    InitializeCriticalSection(&q->cs);
    q->slotsSem = CreateSemaphore(NULL, QUEUE_CAP, QUEUE_CAP, NULL);
    q->itemsSem = CreateSemaphore(NULL, 0, QUEUE_CAP, NULL);
    return (q->slotsSem && q->itemsSem) ? 1 : 0;
}

// Destroy the queue
void q_destroy(DirQueue* q) {
    if (q->items) free(q->items);
    if (q->slotsSem) CloseHandle(q->slotsSem);
    if (q->itemsSem) CloseHandle(q->itemsSem);
    DeleteCriticalSection(&q->cs);
}

int q_push(DirQueue* q, const wchar_t* path) {
    WaitForSingleObject(q->slotsSem, INFINITE);
    EnterCriticalSection(&q->cs);
    wcscpy_s(q->items[q->tail], MAX_PATH_LEN, path);
    q->tail = (q->tail + 1) % QUEUE_CAP;
    LeaveCriticalSection(&q->cs);
    ReleaseSemaphore(q->itemsSem, 1, NULL);
    return 1;
}

int q_pop(DirQueue* q, wchar_t* out, volatile LONG* shutdown) {
    for (;;) {
        DWORD r = WaitForSingleObject(q->itemsSem, 100);
        if (r == WAIT_TIMEOUT) {
            if (*shutdown) return 0;
            continue;
        }
        EnterCriticalSection(&q->cs);
        wcscpy_s(out, MAX_PATH_LEN, q->items[q->head]);
        q->head = (q->head + 1) % QUEUE_CAP;
        LeaveCriticalSection(&q->cs);
        ReleaseSemaphore(q->slotsSem, 1, NULL);
        return 1;
    }
}