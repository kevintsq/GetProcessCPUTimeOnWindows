#ifndef GETPROCESSCPUTIMEONWINDOWS_LIBTIME_H
#define GETPROCESSCPUTIMEONWINDOWS_LIBTIME_H

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <windows.h>
#include "detours.h"

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

#define SHARED_TIMING_INSTANCE_CNT_MEM_NAME "TimingInstanceCountMemory"
char shared_mem_name[40] = "TimingSharedMemory";
char mutex_name[32] = "TimingMutex";
char event_name[32] = "TimingEvent";

#endif //GETPROCESSCPUTIMEONWINDOWS_LIBTIME_H
