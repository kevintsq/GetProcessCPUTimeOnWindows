import subprocess
import time
from threading import Thread

import pythoncom
import win32api
import win32con
import wmi
from win32 import win32process


class Runner(Thread):
    def run(self):
        pythoncom.CoInitialize()
        c = wmi.WMI()
        start_time = time.time()
        with subprocess.Popen(r"java -jar Test.jar") as p:
            result = c.query(f"SELECT * FROM Win32_Process WHERE ParentProcessId = {p.pid}")
            while len(result) == 0 and not p.poll():
                result = c.query(f"SELECT * FROM Win32_Process WHERE ParentProcessId = {p.pid}")
            try:
                handle = win32api.OpenProcess(win32con.PROCESS_ALL_ACCESS, False, result[0].ProcessId)
                p.wait()
                result = win32process.GetProcessTimes(handle)
                print((result['KernelTime'] + result['UserTime']) / 10000000)
                win32api.CloseHandle(handle)
            except:
                print((int(result[0].KernelModeTime) + int(result[0].UserModeTime)) / 10000000)
            print(time.time() - start_time)


if __name__ == "__main__":
    for i in range(5):
        Runner().start()
