# GetProcessCPUTimeOnWindows

This is my implementation of unix's `time` command on Windows. Unix's `time` command recursively records the CPU time consumed by the child processes created by the parent process if the parent process waits its child processes. However, whether the parent process waits or not, Windows's `GetProcessTimes` does not record any CPU time consumed by the child processes created by the parent process (e.g. Python / Java Launchers). Thus, this tool uses Microsoft's `Detour` package to inject a DLL to recursively hook the subprocess' call to `CreateProcessA` and `CreateProcessW`. `DetourCreateProcessWithDll` is called from this tool; it alters the in-memory copy of the subprocess by inserting an import table entry for the DLL. This new import table entry causes the OS loader to load the DLL after the subprocess has started, but before any of the application code can run. The DLL then has a chance to hook target functions in the target process. The DLL records pids of the target process using mapped shared memory, and the main program opens the target process after notified by the DLL. It then resumes the target process, ensuring that the target process information will not be reclaimed by the OS after it exits. Thus, this tool records CPU time accurately. If some operations fail, it falls back to the mode that only records the time consumed by the subprocess specified by the user.

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

To build, MSVC Toolchain is needed. Open `Developer Command Prompt` or `Developer Powershell`, `cd` to this repo's root directory, and then type ` nmake`. `time.exe` and `libtime*.dll` should be in the `bin.*` folder according to the target CPU architecture. Note: `time.exe` and `libtime*.dll` must be in the same folder. Otherwise, it falls back to the mode that only records the time consumed by the subprocess specified by the user.

If you run multiple instances of `time.exe` at the same time, `%uslibtime*.dll` will be generated in the directory where all but the first instance is run, where `%us` is the serial number indicating the order in which the instances are run. This way, the DLL that injects the child process can get the timing process group it currently belongs to via `GetModuleFileName`, and thus open the shared memory, locks, and condition variables that belong to that group. Currently, this is the only way I can think of to recursively pass information to the DLL at the moment. When a concurrently running instance exits, it automatically clears the generated copy of the DLL.

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
