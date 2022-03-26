import win32api
import win32process
import win32con
import win32event
import time

# Reference: https://www.programcreek.com/python/example/8489/win32process.CreateProcess
# Reference: https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/ns-processthreadsapi-startupinfoa

StartupInfo = win32process.STARTUPINFO()
StartupInfo.dwFlags = win32process.STARTF_USESHOWWINDOW
StartupInfo.wShowWindow = win32con.SW_NORMAL
StartupInfo.hStdInput = win32api.GetStdHandle(win32api.STD_INPUT_HANDLE)
StartupInfo.hStdOutput = win32api.GetStdHandle(win32api.STD_OUTPUT_HANDLE)
StartupInfo.hStdError = win32api.GetStdHandle(win32api.STD_ERROR_HANDLE)

command = r"java -jar Test.jar"

startTime = time.time()
TIMEOUT = 1.5

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
win32event.WaitForSingleObject(hProcess, int(TIMEOUT * 1000))
win32api.TerminateProcess(hProcess, 0)
win32event.WaitForSingleObject(hProcess, win32event.INFINITE)

# query cpu time using win32api
sTime = win32process.GetProcessTimes(hProcess)
# print(sTime)
print(f"cputime: {(sTime['KernelTime'] + sTime['UserTime']) / 10000000}")
print(f"realtime: {(sTime['ExitTime'] - sTime['CreationTime']).total_seconds()}")
