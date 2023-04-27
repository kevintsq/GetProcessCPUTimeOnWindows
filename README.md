# GetProcessCPUTimeOnWindows

This is my implementation of unix's `time` command on Windows. Unix's `time` command recursively records the CPU time consumed by the child processes created by the parent process if the parent process waits for its child processes. However, whether the parent process waits or not, Windows's `GetProcessTimes` does not record any CPU time consumed by the child processes created by the parent process (e.g., Python / Java Launchers). Thus, this tool uses Microsoft's `Detour` package to inject a DLL to recursively hook the subprocess' call to `CreateProcessA` and `CreateProcessW`. `DetourCreateProcessWithDllEx` is called from this tool; it alters the in-memory copy of the subprocess by inserting an import table entry for the DLL. This new import table entry causes the OS loader to load the DLL after the subprocess has started, but before any of the application code can run. The DLL then has a chance to hook target functions in the target process. The DLL records pids of the target process using mapped shared memory, and the main program opens the target process after being notified by the DLL. It then resumes the target process, ensuring that the target process information will not be reclaimed by the OS after it exits. Thus, this tool records CPU time accurately. If some operations fail, it falls back to the mode that only records the time consumed by the subprocess specified by the user.

Notes:

1. If any subprocess is created by `ShellExecute` (`ShellExecuteEx`) with the `open` verb, the timing is also correct because `ShellExecute` is a high-level API and the subprocess is created by an internal call to `CreateProcess`. However, if the subprocess is created by `ShellExecute` with the `runas` verb, the time consumed by the subprocess will not be recorded because the user security context is changed. It is also reasonable not to record the time consumed by the subprocess in other user security contexts.
2. The tool supports both 32-bit and 64-bit processes, and the calling chain can be mixed. Whether you use the 32-bit or 64-bit version of this tool, it correctly records the time consumed by both 32-bit and 64-bit subprocesses on a 64-bit OS. If the timing process is 64-bit and the target process is 32-bit or vice versa, `DetourCreateProcessWithDllEx` will start `rundll32.exe` to load the DLL into a helper process that matches the target process temporarily in order to update the target process' import table with the correct DLL.
3. Since this tool already recursively records the CPU time consumed by the child processes created by the parent process, there is no reason to manually use this tool recursively, e.g., using it to time itself. However, you can still get at least one correct result from the outputs even if you do so. Some timing may be 0 if you run the 32-bit version of this tool recursively, or the subprocesses are a mixture of 64-bit and 32-bit processes because of `WOW64` emulation and `rundll32.exe` complexity.

## Native Version

```
Usage:
        time [options] <command>
Options:
        -n      Do Not include any subprocesses created by the command.
        -i      Inaccurate mode. The time consumed by subprocesses created by the command
                that exit before this program opens them will not be recorded.
                Use this if there is a subprocess whose threads cannot be resumed by other processes.
```

To build, MSVC Toolchain is needed. On a 64-bit OS, open `Developer Command Prompt` or `Developer Powershell`, `cd` to this repo's root directory, and then type `nmake`. `time.exe` and `libtime64.dll` should be in the `bin.X64` folder. After that, open `x64_x86 Cross Tools Command Prompt`, `cd` to this repo's root directory, and then type `nmake`. `time.exe` and `libtime32.dll` should be in the `bin.X86` folder. **After that, manually copy `libtime32.dll` from the `bin.X86` folder to the `bin.X64` folder and copy `libtime64.dll` from the `bin.X64` folder to the `bin.X86` folder. IF YOU MODIFY THE CODE AND REBUILD THE TWO VERSIONS OF DLLS, PLEASE REMEMBER TO COPY THEM. Note: `time.exe`, `libtime64.dll`, and `libtime32.dll` must be in the same folder. Otherwise, it falls back to the mode that only records the time consumed by the subprocess specified by the user.**

If you run multiple instances of `time.exe` simultaneously, `%lulibtime64.dll` and `%lulibtime32.dll` will be generated in the directory where all but the first instance is run, where `%lu` is the serial number indicating the order in which the instances are run. This way, the DLL that injects the child process can get the timing process group it currently belongs to via `GetModuleFileName`, and thus open the shared memory, locks, and condition variables that belong to that group. Currently, this is the only way I can think of to recursively pass information to those DLLs at the moment. When a concurrently running instance exits, it automatically clears the generated copy of the DLLs.

## Python C Extension

Not yet completed бн

## Python Legacy Version Requirements and Usage
Python 3.6+
```shell
pip install pywin32 WMI
```

```shell
python time.py <command to execute>
```

If you want to get the CPU time consumed by the subprocesses spawned by the parent process, especially for Java programs, please use:
```shell
python time_spawned_by_parent.py <command to execute>
```