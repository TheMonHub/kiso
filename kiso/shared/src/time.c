#include "kiso/time.h"

#ifdef _WIN32

#include <windows.h>

double GetMonotonicTime() {
    static LARGE_INTEGER frequency;
    static int initialized = 0;

    if (!initialized) {
        QueryPerformanceFrequency(&frequency);
        initialized = 1;
    }

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    return (double)counter.QuadPart /
           (double)frequency.QuadPart;
}

#else

#ifndef _XOPEN_SOURCE
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
#define _XOPEN_SOURCE 700
#endif
#include <time.h>
#include <unistd.h>

double GetMonotonicTime() {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (double)ts.tv_sec +
           (double)ts.tv_nsec / 1000000000.0;
}

#endif