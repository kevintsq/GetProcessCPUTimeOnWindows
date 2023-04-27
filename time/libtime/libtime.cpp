#include "libtime.h"
#include <ctype.h>
#include <psapi.h>
//BOOL is_loaded = FALSE;

HANDLE mutex;
HANDLE event;
HANDLE map_file;
HANDLE timing_instance_cnt_map_file;

#define PATH_LEN 32768
char dll_path[PATH_LEN];

void PrintLastError(const char *funcName) {
	char *msg;
	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char *) &msg, 0, NULL);
	fprintf(stderr, "%s failed: %s", funcName, msg);
	LocalFree(msg);
}

BOOL(__stdcall* Real_CreateProcessA)(LPCSTR a0,
	LPSTR a1,
	LPSECURITY_ATTRIBUTES a2,
	LPSECURITY_ATTRIBUTES a3,
	BOOL a4,
	DWORD a5,
	LPVOID a6,
	LPCSTR a7,
	LPSTARTUPINFOA a8,
	LPPROCESS_INFORMATION a9)
	= CreateProcessA;

BOOL(__stdcall* Real_CreateProcessW)(LPCWSTR a0,
	LPWSTR a1,
	LPSECURITY_ATTRIBUTES a2,
	LPSECURITY_ATTRIBUTES a3,
	BOOL a4,
	DWORD a5,
	LPVOID a6,
	LPCWSTR a7,
	LPSTARTUPINFOW a8,
	LPPROCESS_INFORMATION a9)
	= CreateProcessW;

BOOL __stdcall Mine_CreateProcessA(LPCSTR lpApplicationName,
	LPSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCSTR lpCurrentDirectory,
	LPSTARTUPINFOA lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation)
{
    PROCESS_INFORMATION procInfo;
	if (lpProcessInformation == NULL) {
		lpProcessInformation = &procInfo;
		ZeroMemory(&procInfo, sizeof(procInfo));
	}

	BOOL rv = DetourCreateProcessWithDllExA(lpApplicationName,
                                            lpCommandLine,
                                            lpProcessAttributes,
                                            lpThreadAttributes,
                                            bInheritHandles,
                                            dwCreationFlags | shared->proc_creation_flags,
                                            lpEnvironment,
                                            lpCurrentDirectory,
                                            lpStartupInfo,
                                            lpProcessInformation,
                                            dll_path,
                                            Real_CreateProcessA);
    if (rv) {
        WaitForSingleObject(mutex, INFINITE);
        if (shared->cnt < MAX_IDS) {
            shared->ids[shared->cnt++] = /*(Id)*/ {lpProcessInformation->dwProcessId, lpProcessInformation->dwThreadId};
            SetEvent(event);
        }
        else {
            fprintf(stderr, "WARNING: Too many subprocesses. Measurement will be inaccurate.\n");
        }
        ReleaseMutex(mutex);
    }
	return rv;
}

BOOL __stdcall Mine_CreateProcessW(LPCWSTR lpApplicationName,
	LPWSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCWSTR lpCurrentDirectory,
	LPSTARTUPINFOW lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation)
{
	PROCESS_INFORMATION procInfo;
	if (lpProcessInformation == NULL) {
		lpProcessInformation = &procInfo;
		ZeroMemory(&procInfo, sizeof(procInfo));
	}

	BOOL rv = DetourCreateProcessWithDllExW(lpApplicationName,
                                            lpCommandLine,
                                            lpProcessAttributes,
                                            lpThreadAttributes,
                                            bInheritHandles,
                                            dwCreationFlags | shared->proc_creation_flags,
                                            lpEnvironment,
                                            lpCurrentDirectory,
                                            lpStartupInfo,
                                            lpProcessInformation,
                                            dll_path,
                                            Real_CreateProcessW);
    if (rv) {
        WaitForSingleObject(mutex, INFINITE);
        if (shared->cnt < MAX_IDS) {
            shared->ids[shared->cnt++] = /*(Id)*/ {lpProcessInformation->dwProcessId, lpProcessInformation->dwThreadId};
            SetEvent(event);
        }
        else {
            fprintf(stderr, "WARNING: Too many subprocesses. Measurement will be inaccurate.\n");
        }
        ReleaseMutex(mutex);
    }
	return rv;
}

static char *DetectRealName(char *name) {
	char *name_begin = name;
	// Move to end of name.
	while (*name) {
		name++;
	}
	// Move back through A-Za-z0-9 names.
	while (name > name_begin &&
		((name[-1] >= 'A' && name[-1] <= 'Z') ||
			(name[-1] >= 'a' && name[-1] <= 'z') ||
			(name[-1] >= '0' && name[-1] <= '9'))) {
		name--;
	}
	return name;
}

static void Dump(unsigned char *bytes, int nBytes, unsigned char *target) {
	char buffer[256];
	char *p = buffer;

	for (int n = 0; n < nBytes; n += 12) {
		p += sprintf(p, "  %p: ", bytes + n);
		for (int m = n; m < n + 12; m++) {
			if (m >= nBytes) {
				p += sprintf(p, "   ");
			}
			else {
				p += sprintf(p, "%02x ", bytes[m]);
			}
		}
		if (n == 0) {
			p += sprintf(p, "[%p]", target);
		}
		p += sprintf(p, "\n");
	}

	fprintf(stderr, "%s\n", buffer);
}

static void Decode(unsigned char *code, int nInst) {
	unsigned char *source = code;
	unsigned char *end;
	unsigned char *target;
	for (int n = 0; n < nInst; n++) {
		target = NULL;
		end = (unsigned char *) DetourCopyInstruction(NULL, NULL, (void *) source, (void **) &target, NULL);
		Dump(source, (int) (end - source), target);
		source = end;

		if (target != NULL) {
			break;
		}
	}
}

void DetAttach(void **pReal, void *mine, char *name) {
	void *real = NULL;
	if (pReal == NULL) {
		pReal = &real;
	}

	long error = DetourAttach(pReal, mine);
	if (error != 0) {
		fprintf(stderr, "Attach failed: %s: error %ld\n", DetectRealName(name), error);
		Decode((unsigned char *) *pReal, 3);
	}
}

void DetDetach(void **pReal, void *mine, char *name) {
    (void) name;
    DetourDetach(pReal, mine);
}

#define ATTACH(x)       DetAttach(&(void *)Real_##x,Mine_##x,#x)
#define DETACH(x)       DetDetach(&(void *)Real_##x,Mine_##x,#x)

long AttachDetours() {
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	ATTACH(CreateProcessA);
	ATTACH(CreateProcessW);
	void **failed_pointer = NULL;
	long error = DetourTransactionCommitEx(&failed_pointer);
	if (error != 0) {
		fprintf(stderr, "ERROR: Attach transaction failed to commit. Error %ld (%p/%p)\n",
			error, failed_pointer, *failed_pointer);
		return error;
	}
	return 0;
}

long DetachDetours() {
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DETACH(CreateProcessA);
	DETACH(CreateProcessW);
	return DetourTransactionCommit();
}

void PrintProcessName() {
    unsigned long pid = GetCurrentProcessId();
    HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    HMODULE moduleHandles[32];
    DWORD moduleSize;
    if (EnumProcessModulesEx(process, moduleHandles, sizeof(moduleHandles) / sizeof(HMODULE), &moduleSize, LIST_MODULES_ALL)) {
        char moduleFileName[MAX_PATH];
        if (GetModuleFileNameExA(process, moduleHandles[0], moduleFileName, sizeof(moduleFileName) / sizeof(char))) {
            printf("Loaded by process: %s\n", moduleFileName);
        }
    } else {
        PrintLastError("EnumProcessModules");
    }
}

BOOL ProcessAttach(HMODULE dll) {
//    PrintProcessName();
    unsigned long len = GetModuleFileNameA(dll, dll_path, PATH_LEN);
    unsigned long error = GetLastError();
    if (len == 0 || error == ERROR_INSUFFICIENT_BUFFER) {
        char *msg;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char *) &msg, 0, NULL);
        fprintf(stderr, "GetModuleFileName failed: %s\n", msg);
        LocalFree(msg);
        return FALSE;
    }

    char *num_start = strrchr(dll_path, '\\');
    char *num_end;
    for (num_end = ++num_start; isdigit(*num_end); num_end++);
    strncat(shared_mem_name, num_start, num_end - num_start);
    strncat(mutex_name, num_start, num_end - num_start);
    strncat(event_name, num_start, num_end - num_start);

    // In order to avoid zombie subprocesses affecting other timing processes,
    // the child process also needs to open the shared memory so that it will be reclaimed
    // only when there are no timing processes running, i.e., shared_timing_instance_cnt will be cleared.
    timing_instance_cnt_map_file = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, SHARED_TIMING_INSTANCE_CNT_MEM_NAME);
    if (!timing_instance_cnt_map_file) {
        PrintLastError("OpenFileMapping");
        fprintf(stderr, "ERROR: The main program must run first to create the shared memory "
                        SHARED_TIMING_INSTANCE_CNT_MEM_NAME ".\n");
        return FALSE;
    }
    map_file = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, shared_mem_name);
    if (!map_file) {
        PrintLastError("OpenFileMapping");
        fprintf(stderr, "ERROR: The main program must run first to create the shared memory %s.\n", shared_mem_name);
        return FALSE;
    }
    shared = (Shared *) MapViewOfFile(map_file, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Shared));
    if (!shared) {
        PrintLastError("MapViewOfFile");
        return FALSE;
    }
    mutex = OpenMutexA(MUTEX_ALL_ACCESS, FALSE, mutex_name);
    if (!mutex) {
        PrintLastError("OpenMutex");
        fprintf(stderr, "ERROR: The main program must run first to create the mutex %s.\n", mutex_name);
        return FALSE;
    }
    event = OpenEventA(EVENT_ALL_ACCESS, FALSE, event_name);
    if (!event) {
        PrintLastError("OpenEvent");
        fprintf(stderr, "ERROR: The main program must run first to create the mutex %s.\n", event_name);
        return FALSE;
    }

	AttachDetours();

	return TRUE;
}

BOOL ProcessDetach() {
	DetachDetours();
	if (mutex) {
		CloseHandle(mutex);
	}
	if (event) {
		CloseHandle(event);
	}
	if (shared) {
		UnmapViewOfFile(shared);
	}
	if (map_file) {
		CloseHandle(map_file);
	}
    if (timing_instance_cnt_map_file) {
        CloseHandle(timing_instance_cnt_map_file);
    }
	return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, PVOID reserved) {
	(void) module;
	(void) reserved;

	if (DetourIsHelperProcess()) {
//        if (!is_loaded) {
//            printf("(Helper Process) ");
//            PrintProcessName();
//        }
//        is_loaded = TRUE;
		return TRUE;
	}

	switch (reason) {
	case DLL_PROCESS_ATTACH:
		DetourRestoreAfterWith();
		return ProcessAttach(module);
	case DLL_PROCESS_DETACH:
		return ProcessDetach();
    default:
        return TRUE;
	}
}
