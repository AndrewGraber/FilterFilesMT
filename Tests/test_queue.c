#include <stdio.h>
#include "test_queue.h"

int test_queue_st(void) {
    wprintf(L"=== Single-threaded Queue Test ===\n");

    DirQueue q;
    q.items = malloc(sizeof(wchar_t[QUEUE_CAP][MAX_PATH_LEN]));
    if (!q.items) { fwprintf(stderr, L"Heap allocation failed\n"); return 1; }

    InitializeCriticalSection(&q.cs);
    q.head = q.tail = 0;
    q.slotsSem = CreateSemaphore(NULL, QUEUE_CAP, QUEUE_CAP, NULL);
    q.itemsSem = CreateSemaphore(NULL, 0, QUEUE_CAP, NULL);

    wchar_t shutdownFlag = 0;
    const int N = 100;
    int failed = 0;

    // Push items
    for (int i = 0; i < N; i++) {
        wchar_t buf[MAX_PATH_LEN];
        swprintf(buf, MAX_PATH_LEN, L"item-%d", i);
        if (!q_push(&q, buf)) failed++;
    }

    // Pop items
    for (int i = 0; i < N; i++) {
        wchar_t out[MAX_PATH_LEN];
        if (!q_pop(&q, out, (volatile LONG*)&shutdownFlag)) { failed++; break; }
        wchar_t expected[MAX_PATH_LEN];
        swprintf(expected, MAX_PATH_LEN, L"item-%d", i);
        if (wcscmp(out, expected) != 0) {
            wprintf(L"[FAIL] Expected '%s', got '%s'\n", expected, out);
            failed++;
        }
    }

    CloseHandle(q.slotsSem);
    CloseHandle(q.itemsSem);
    DeleteCriticalSection(&q.cs);
    free(q.items);

    if (!failed) wprintf(L"[PASS] Single-threaded queue test passed.\n");
    return failed;
}

/* ---------------- Multithreaded test ---------------- */
typedef struct {
    DirQueue* q;
    int start;
    int end;
    volatile LONG* shutdown;
} TestThreadArg;

DWORD WINAPI producer(LPVOID param) {
    TestThreadArg* a = (TestThreadArg*)param;
    for (int i = a->start; i < a->end; i++) {
        wchar_t buf[MAX_PATH_LEN];
        swprintf(buf, MAX_PATH_LEN, L"item-%d", i);
        q_push(a->q, buf);
    }
    return 0;
}

DWORD WINAPI consumer(LPVOID param) {
    TestThreadArg* a = (TestThreadArg*)param;
    wchar_t out[MAX_PATH_LEN];
    while (q_pop(a->q, out, a->shutdown)) {
        // Optional: check format or count
    }
    return 0;
}

int test_queue_mt(void) {
    wprintf(L"=== Multithreaded Queue Test ===\n");

    DirQueue q;
    q.items = malloc(sizeof(wchar_t[QUEUE_CAP][MAX_PATH_LEN]));
    if (!q.items) { fwprintf(stderr, L"Heap allocation failed\n"); return 1; }

    InitializeCriticalSection(&q.cs);
    q.head = q.tail = 0;
    q.slotsSem = CreateSemaphore(NULL, QUEUE_CAP, QUEUE_CAP, NULL);
    q.itemsSem = CreateSemaphore(NULL, 0, QUEUE_CAP, NULL);

    volatile LONG shutdownFlag = 0;
    HANDLE producers[2], consumers[2];
    TestThreadArg pargs[2], cargs[2];

    // Start producers
    for (int i = 0; i < 2; i++) {
        pargs[i].q = &q;
        pargs[i].start = i * 50;
        pargs[i].end = (i+1) * 50;
        pargs[i].shutdown = &shutdownFlag;
        producers[i] = CreateThread(NULL, 0, producer, &pargs[i], 0, NULL);
    }

    // Start consumers
    for (int i = 0; i < 2; i++) {
        cargs[i].q = &q;
        cargs[i].shutdown = &shutdownFlag;
        consumers[i] = CreateThread(NULL, 0, consumer, &cargs[i], 0, NULL);
    }

    // Wait for producers to finish
    WaitForMultipleObjects(2, producers, TRUE, INFINITE);

    // Signal shutdown
    shutdownFlag = 1;

    // Release semaphores to wake consumers if blocked
    for (int i = 0; i < 2; i++) ReleaseSemaphore(q.itemsSem, 1, NULL);

    WaitForMultipleObjects(2, consumers, TRUE, INFINITE);

    CloseHandle(q.slotsSem);
    CloseHandle(q.itemsSem);
    DeleteCriticalSection(&q.cs);
    free(q.items);

    wprintf(L"[PASS] Multithreaded queue test completed.\n");
    return 0;
}