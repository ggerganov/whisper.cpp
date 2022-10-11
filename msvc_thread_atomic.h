#pragma once
#include <Windows.h>

typedef volatile LONG atomic_int;
typedef atomic_int atomic_bool;

static void atomic_store(atomic_int* ptr, LONG val) {
    InterlockedExchange(ptr, val);
}
static LONG atomic_load(atomic_int* ptr) {
    return InterlockedCompareExchange(ptr, 0, 0);
}
static LONG atomic_fetch_add(atomic_int* ptr, LONG inc) {
    return InterlockedExchangeAdd(ptr, inc);
}
static LONG atomic_fetch_sub(atomic_int* ptr, LONG dec) {
    return atomic_fetch_add(ptr, -(dec));
}

typedef HANDLE pthread_t;

typedef DWORD thread_ret_t;
static int pthread_create(pthread_t* out, void* unused, thread_ret_t(*func)(void*), void* arg) {
    out = CreateThread(NULL, 0, func, arg, 0, NULL);
    return out != NULL;
}

static int pthread_join(pthread_t thread, void* unused) {
    return (int) WaitForSingleObject(thread, INFINITE);
}

