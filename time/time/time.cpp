#include "libtime.h"

#define SHARED_TIMING_INSTANCE_CNT_MEM_NAME "TimingInstanceCountMemory"
#define TIMING_INSTANCE_CNT_MUTEX_NAME "TimingInstanceCountMutex"

char dll_name[20];
#if DETOURS_64BIT
#define DLL_NAME "libtime64.dll"
#else
#define DLL_NAME "libtime32.dll"
#endif

unsigned short *shared_timing_instance_cnt;

HANDLE subprocesses[MAX_IDS];
int n_subprocess;
BOOL finished;
HANDLE event;

unsigned long PrintLastError(const char *funcName) {
    LPSTR msg;
    unsigned long error = GetLastError();
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msg, 0, NULL);
    fprintf(stderr, "%s failed: %s", funcName, msg);
    LocalFree(msg);
    return error;
}

static inline void PrintFallbackWarning() {
    fprintf(stderr, "WARNING: Falling back to inaccurate mode. Only the time of one child process created directly will be counted.\n");
}

//////////////////////////////////////////////////////////////////////////////
//
//  This code verifies that the named DLL has been configured correctly
//  to be imported into the target process.  DLLs must export a function with
//  ordinal #1 so that the import table touch-up magic works.
//
typedef struct export_context_t {
    BOOL    fHasOrdinal1;
    ULONG   nExports;
} ExportContext;

static BOOL ExportCallback(_In_opt_ void *context,
                           _In_ unsigned long ordinal,
                           _In_opt_ const char *symbol,
                           _In_opt_ void *target)
{
    (void) symbol;
    (void) target;

    ExportContext *export_context = (ExportContext *) context;

    if (ordinal == 1) {
        export_context->fHasOrdinal1 = TRUE;
    }
    export_context->nExports++;

    return TRUE;
}

unsigned long OpenSubprocesses(void* ignored) {
    (void) ignored;

    HANDLE mutex = CreateMutexA(NULL, FALSE, mutex_name);
    if (!mutex) {
        PrintLastError("OpenMutex");
        PrintFallbackWarning();
        return EXIT_FAILURE;
    }
    event = CreateEventA(NULL, FALSE, FALSE, event_name);
    if (!event) {
        PrintLastError("OpenEvent");
        PrintFallbackWarning();
        return EXIT_FAILURE;
    }

    while (!finished) {
        WaitForSingleObject(mutex, INFINITE); // 获取互斥锁
        while (shared->cnt == 0 && !finished) {
            ReleaseMutex(mutex); // 解锁互斥锁
            WaitForSingleObject(event, INFINITE); // 等待条件变量
            WaitForSingleObject(mutex, INFINITE); // 重新获取互斥锁
        }
        if (shared->cnt > 0) { // 临界区操作
            Id id = shared->ids[--shared->cnt];

            // 需在 ResumeThread 前 OpenProcess，否则可能会出现进程已退出的情况
            HANDLE subprocess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, id.pid);

            if (shared->proc_creation_flags & CREATE_SUSPENDED) {
                // 精确模式子进程会在 OpenProcess 前挂起，需尽快 ResumeThread，以减少独占互斥锁的时间
                HANDLE thread = OpenThread(THREAD_ALL_ACCESS, FALSE, id.tid);
                if (!thread) {
                    PrintLastError("OpenThread");
                    PrintFallbackWarning();
                    ReleaseMutex(mutex); // 解锁互斥锁
                    TerminateProcess(subprocess, EXIT_FAILURE);  // 需要终止因为它无法自己唤醒
                    return EXIT_FAILURE;
                }
                if (ResumeThread(thread) == -1) {
                    PrintLastError("ResumeThread");
                    PrintFallbackWarning();
                    ReleaseMutex(mutex); // 解锁互斥锁
                    TerminateProcess(subprocess, EXIT_FAILURE);  // 需要终止因为它无法自己唤醒
                    return EXIT_FAILURE;
                }
                CloseHandle(thread);  // must close in loop
            }

            ReleaseMutex(mutex); // 需在 ResumeThread 后解锁互斥锁
            if (!subprocess) {
                PrintLastError("OpenProcess");
                fprintf(stderr, "WARNING: Open subprocess %lu failed. Measurement will be inaccurate.\n", id.pid);
            }
            else {
                subprocesses[n_subprocess++] = subprocess;
            }
        }
        else {
            ReleaseMutex(mutex); // 解锁互斥锁
        }
    }
    return EXIT_SUCCESS;
}

void ProcessTimes(FILETIME creation_time, FILETIME exit_time, FILETIME kernel_time, FILETIME user_time,
                  double* elapsed_time, double* kernel_time_sec, double* user_time_sec) {
    if (elapsed_time != NULL) {
        *elapsed_time += ((double) exit_time.dwLowDateTime - (double) creation_time.dwLowDateTime) / 10000000.0 +
                         ((double) exit_time.dwHighDateTime - (double) creation_time.dwHighDateTime) * 429.4967296;
    }
    if (kernel_time_sec != NULL) {
        *kernel_time_sec += (double) kernel_time.dwLowDateTime / 10000000.0 + (double) kernel_time.dwHighDateTime * 429.4967296;
    }
    if (user_time_sec != NULL) {
        *user_time_sec += (double) user_time.dwLowDateTime / 10000000.0 + (double) user_time.dwHighDateTime * 429.4967296;
    }
}

int main(int argc, char *argv[]) {
    unsigned long exit_code = EXIT_FAILURE;

    BOOL accurate_mode = TRUE;
    BOOL include_subs = TRUE;
    BOOL need_help = FALSE;
    int optind;
    for (optind = 1; optind < argc && (argv[optind][0] == '-' || argv[optind][0] == '/'); optind++) {
        char opt = argv[optind][1];
        switch (opt) {
            case 'n':
            case 'N':
                include_subs = FALSE;
                break;
            case 'i':
            case 'I':
                accurate_mode = FALSE;
                break;
            default:
                need_help = TRUE;
                break;
        }
    }
    if (optind >= argc) {
        need_help = TRUE;
    }
    if (need_help) {
        fprintf(stderr, "Usage:\n"
                "\ttime [options] <command>\n"
                "Options:\n"
                "\t-n\tDo Not include any subprocesses created by the command.\n"
                "\t-i\tInaccurate mode. The time consumed by subprocesses created by the command\n"
                "\t  \tthat exit before this program opens them will not be recorded.\n"
                "\t  \tUse this if there is a subprocess whose threads cannot be resumed by other processes.\n");
        return 9001;
    }

    // Combine command line arguments into one string.
    size_t command_line_length = 0;
    for (int i = optind; i < argc; i++) {
        if (strchr(argv[i], ' ') != NULL || strchr(argv[i], '\t') != NULL) {
            command_line_length += strlen(argv[i]) + 3;  // +3 for two quotes and space or null terminator
        } else {
            command_line_length += strlen(argv[i]) + 1;  // +1 for space or null terminator
        }
    }
    char *command_line = (char *) malloc(command_line_length);
    command_line[0] = '\0';
    for (int i = optind; i < argc; i++) {
        if (strchr(argv[i], ' ') != NULL || strchr(argv[i], '\t') != NULL) {
            strcat(command_line, "\"");
            strcat(command_line, argv[i]);
            strcat(command_line, "\"");
        } else {
            strcat(command_line, argv[i]);
        }
        if (i < argc - 1) {
            strcat(command_line, " ");
        }
    }

    HANDLE map_file, timing_instance_cnt_map_file, timing_thread = NULL;
    unsigned short timing_instance_cnt = 0;
    unsigned long dll_path_len;
    char *dll_path;
    HMODULE dll;
    ExportContext export_context;
    STARTUPINFOA startup_info = {0};
    startup_info.cb = sizeof(startup_info);
    PROCESS_INFORMATION process_info;

    if (!include_subs) {
        goto no_include_subs;
    }

    timing_instance_cnt_map_file = CreateFileMappingA(NULL, NULL, PAGE_READWRITE, 0, sizeof(unsigned short), SHARED_TIMING_INSTANCE_CNT_MEM_NAME);
    if (!timing_instance_cnt_map_file) {
        PrintLastError("CreateFileMapping");
        PrintFallbackWarning();
        goto no_include_subs;
    }
    shared_timing_instance_cnt = (unsigned short *) MapViewOfFile(timing_instance_cnt_map_file, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (!shared_timing_instance_cnt) {
        PrintLastError("MapViewOfFile");
        PrintFallbackWarning();
        goto no_include_subs;
    }
    HANDLE mutex = CreateMutexA(NULL, FALSE, TIMING_INSTANCE_CNT_MUTEX_NAME);
    if (!mutex) {
        PrintLastError("CreateMutex");
        PrintFallbackWarning();
        goto no_include_subs;
    }
    WaitForSingleObject(mutex, INFINITE);
    timing_instance_cnt = *shared_timing_instance_cnt;
    if (timing_instance_cnt == USHRT_MAX) {
        fprintf(stderr, "WARNING: Too many instances. Falling back to inaccurate mode. Only the time of one child process created directly will be counted.\n");
        ReleaseMutex(mutex);
        goto no_include_subs;
    }
    (*shared_timing_instance_cnt)++;
    ReleaseMutex(mutex);
    if (timing_instance_cnt > 0) {
        sprintf(shared_mem_name, "%s%hu", shared_mem_name, timing_instance_cnt);
        sprintf(mutex_name, "%s%hu", mutex_name, timing_instance_cnt);
        sprintf(event_name, "%s%hu", event_name, timing_instance_cnt);
        sprintf(dll_name, "%hu" DLL_NAME, timing_instance_cnt);
        if (!CopyFileA(DLL_NAME, dll_name, TRUE)) {
            PrintLastError("CopyFile");
            PrintFallbackWarning();
            goto no_include_subs;
        }
    } else {
        strcpy(dll_name, DLL_NAME);
    }

//    map_file = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, SHARED_MEM_NAME);
//    if (map_file) {
//        fprintf(stderr, "WARNING: Too many instances. Falling back to inaccurate mode. Only the time of one child process created directly will be counted.\n");
//        goto no_include_subs;
//    }

    dll_path_len = GetFullPathNameA(dll_name, 0, NULL, NULL);
    dll_path = (char *) malloc(dll_path_len);
    GetFullPathNameA(dll_name, dll_path_len, dll_path, NULL);
    dll = LoadLibraryExA(dll_path, NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (dll == NULL) {
        PrintLastError("LoadLibraryEx");
        PrintFallbackWarning();
        goto no_include_subs;
    }
    DetourEnumerateExports(dll, &export_context, ExportCallback);
    FreeLibrary(dll);  // must free before CreateProcess
    if (!export_context.fHasOrdinal1) {
        fprintf(stderr, "ERROR: %s does not export ordinal #1.\n", dll_path);
        PrintFallbackWarning();
        goto no_include_subs;
    }

    map_file = CreateFileMappingA(NULL, NULL, PAGE_READWRITE, 0, sizeof(Shared), shared_mem_name);
    if (!map_file) {
        PrintLastError("CreateFileMapping");
        PrintFallbackWarning();
        goto no_include_subs;
    }
    shared = (Shared *) MapViewOfFile(map_file, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Shared));
    if (!shared) {
        PrintLastError("MapViewOfFile");
        PrintFallbackWarning();
        goto no_include_subs;
    }
    if (accurate_mode) {
        shared->proc_creation_flags = CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED;
    } else {
        shared->proc_creation_flags = CREATE_DEFAULT_ERROR_MODE;
    }

    timing_thread = CreateThread(NULL, 0, (PTHREAD_START_ROUTINE) OpenSubprocesses, NULL, 0, NULL);
    if (!timing_thread) {
        PrintLastError("CreateThread");
        PrintFallbackWarning();
        goto no_include_subs;
    }

    if (!DetourCreateProcessWithDllExA(NULL, command_line, NULL, NULL, TRUE, shared->proc_creation_flags, NULL, NULL,
                                       &startup_info, &process_info, dll_path, NULL)) {
        unsigned long error = PrintLastError("DetourCreateProcessWithDllEx");
        if (error == ERROR_INVALID_HANDLE) {
#if DETOURS_64BIT
            fprintf(stderr, "ERROR: Can't detour a 32-bit target process from a 64-bit parent process.\n");
#else
            fprintf(stderr, "ERROR: Can't detour a 64-bit target process from a 32-bit parent process.\n");
#endif
        }
        PrintFallbackWarning();
        goto no_include_subs;
    }

    if (shared->proc_creation_flags & CREATE_SUSPENDED) {
        if (!ResumeThread(process_info.hThread)) {
            PrintLastError("ResumeThread");
            TerminateProcess(process_info.hProcess, EXIT_FAILURE);
            PrintFallbackWarning();
            goto no_include_subs;
        }
    }

    goto normal;

no_include_subs:
    ZeroMemory(&startup_info, sizeof(startup_info));
    if (CreateProcessA(NULL, command_line, NULL, NULL, FALSE, 0, NULL, NULL, &startup_info, &process_info)) {
        WaitForSingleObject(process_info.hProcess, INFINITE);
    }
    else {
        PrintLastError("CreateProcess");
        return EXIT_FAILURE;
    }

normal:
    WaitForSingleObject(process_info.hProcess, INFINITE);
    finished = TRUE;
    SetEvent(event);

    FILETIME creation_time, exit_time, kernel_time, user_time;
    double elapsed_time = 0, kernel_time_sec = 0, user_time_sec = 0;
    if (!GetProcessTimes(process_info.hProcess, &creation_time, &exit_time, &kernel_time, &user_time)) {
        PrintLastError("GetProcessTimes");
        fprintf(stderr, "WARNING: Skipping process %lu.\n", process_info.dwProcessId);
    }
    else {
        ProcessTimes(creation_time, exit_time, kernel_time, user_time, &elapsed_time, &kernel_time_sec, &user_time_sec);
    }
    if (timing_thread) {
        WaitForSingleObject(timing_thread, INFINITE);
        unsigned long return_value = EXIT_SUCCESS;
        if (!GetExitCodeThread(timing_thread, &return_value)) {
            PrintLastError("GetExitCodeThread");
        }
        if (return_value != EXIT_SUCCESS) {
            timing_thread = NULL;
            goto no_include_subs;
        }
        for (int i = 0; i < n_subprocess; i++) {
            if (!GetProcessTimes(subprocesses[i], &creation_time, &exit_time, &kernel_time, &user_time)) {
                PrintLastError("GetProcessTimes");
                fprintf(stderr, "WARNING: Skipping process %lu.\n", GetProcessId(subprocesses[i]));
            }
            else {
                ProcessTimes(creation_time, exit_time, kernel_time, user_time, NULL, &kernel_time_sec, &user_time_sec);
            }
        }
    }
    double cpu_time_sec = kernel_time_sec + user_time_sec;
    printf("real\t%fs\n", elapsed_time);
    printf("user\t%fs\n", user_time_sec);
    printf("sys\t%fs\n", kernel_time_sec);
    printf("cpu\t%fs\n", cpu_time_sec);

    if (!GetExitCodeProcess(process_info.hProcess, &exit_code)) {
        PrintLastError("GetExitCodeProcess");
    }
    printf("Process finished with exit code %lx\n", exit_code);

    if (timing_instance_cnt > 0) {
        if (!DeleteFileA(dll_name)) {
            PrintLastError("DeleteFile");
            fprintf(stderr, "INFO: %s can't be deleted. You can delete it manually afterwards.\n", dll_name);
        }
    }
    return exit_code;
}
