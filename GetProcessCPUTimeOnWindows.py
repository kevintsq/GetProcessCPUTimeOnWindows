import win32api
import win32process
import win32con
import time

# Reference: https://www.programcreek.com/python/example/8489/win32process.CreateProcess
# Reference: https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/ns-processthreadsapi-startupinfoa

StartupInfo = win32process.STARTUPINFO()
StartupInfo.dwFlags = win32process.STARTF_USESHOWWINDOW
StartupInfo.wShowWindow = win32con.SW_NORMAL
StartupInfo.hStdInput = win32api.GetStdHandle(win32api.STD_INPUT_HANDLE)
StartupInfo.hStdOutput = win32api.GetStdHandle(win32api.STD_OUTPUT_HANDLE)
StartupInfo.hStdError = win32api.GetStdHandle(win32api.STD_ERROR_HANDLE)

command = "ping -n 4 127.0.0.1"

startTime = time.time()
TIMEOUT = 1

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

# waiting for subprocess to exit, using polling mode
while win32process.GetExitCodeProcess(hProcess) == win32con.STILL_ACTIVE:
    if time.time() - startTime > TIMEOUT:
        print(time.time() - startTime)
        win32api.TerminateProcess(hProcess, 0)
    time.sleep(0.1) # there is no wait method in windows (maybe)

# query cpu time using win32api
sTime = win32process.GetProcessTimes(hProcess)
print(sTime)
print(f"cputime: {(sTime['KernelTime'] + sTime['UserTime']) / 10000000}")

