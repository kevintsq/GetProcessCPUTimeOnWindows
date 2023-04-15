"""GetProcessCPUTimeUsingWin32API"""

import sys

import win32api
import win32process
import win32con
import win32event

# Reference: https://www.programcreek.com/python/example/8489/win32process.CreateProcess
# Reference: https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/ns-processthreadsapi-startupinfoa

StartupInfo = win32process.STARTUPINFO()
StartupInfo.dwFlags = win32process.STARTF_USESHOWWINDOW
StartupInfo.wShowWindow = win32con.SW_NORMAL
StartupInfo.hStdInput = win32api.GetStdHandle(win32api.STD_INPUT_HANDLE)
StartupInfo.hStdOutput = win32api.GetStdHandle(win32api.STD_OUTPUT_HANDLE)
StartupInfo.hStdError = win32api.GetStdHandle(win32api.STD_ERROR_HANDLE)

command = sys.argv[1:]
try:
    if not isinstance(command, str):
        command = " ".join(sys.argv[1:])
except:
    raise Exception("command must be a non-empty string, a list or a tuple of strings")
else:
    if not command:
        raise Exception("command must be a non-empty string, a list or a tuple of strings")

# create subprocess using win32api
hProcess, hThread, dwProcessId, dwThreadId = win32process.CreateProcess(
                None,
                command,
                None,
                None,
                0,
                win32process.NORMAL_PRIORITY_CLASS,
                None,
                None,
                StartupInfo)

# waiting for subprocess to exit
win32event.WaitForSingleObject(hProcess, win32event.INFINITE)

# query return value using win32api
rtv = win32process.GetExitCodeProcess(hProcess)

# query cpu time using win32api
processTimes = win32process.GetProcessTimes(hProcess)
rtime = (processTimes['ExitTime'] - processTimes['CreationTime']).total_seconds()
utime = processTimes['UserTime'] / 10000000
stime = processTimes['KernelTime'] / 10000000
print(f"real\t{rtime}s")
print(f"user\t{utime}s")
print(f"sys\t{stime}s")
print(f"cpu\t{ctime}s")
print(f"Process finished with exit code{rtv}")
