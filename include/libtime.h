#ifndef GETPROCESSCPUTIMEONWINDOWS_LIBTIME_H
#define GETPROCESSCPUTIMEONWINDOWS_LIBTIME_H

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <windows.h>
#include "detours.h"

#define SHARED_MEM_NAME "TimingSharedMemory"
#define MUTEX_NAME "TimingMutex"
#define EVENT_NAME "TimingEvent"
#define MAX_IDS 8191
typedef struct id_t {
    unsigned long pid;
    unsigned long tid;
} Id;
typedef struct shared_t {
    Id ids[MAX_IDS];
    unsigned long cnt;
    unsigned long proc_creation_flags;
} Shared;

Shared* shared;

#endif //GETPROCESSCPUTIMEONWINDOWS_LIBTIME_H
