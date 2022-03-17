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
        wmi_service = wmi.WMI()
        start_time = time.time()
        with subprocess.Popen(r"java -jar Test.jar") as process:
            process_info = wmi_service.query(f"SELECT * FROM Win32_Process WHERE ParentProcessId = {process.pid}")
            while len(process_info) == 0 and not process.poll():
                process_info = wmi_service.query(f"SELECT * FROM Win32_Process WHERE ParentProcessId = {process.pid}")
            try:
                handle = win32api.OpenProcess(win32con.PROCESS_ALL_ACCESS, False, process_info[0].ProcessId)
                process.wait()
                process_info = win32process.GetProcessTimes(handle)
                print((process_info['KernelTime'] + process_info['UserTime']) / 10000000)
                win32api.CloseHandle(handle)
            except:
                print((int(process_info[0].KernelModeTime) + int(process_info[0].UserModeTime)) / 1000)
            print(time.time() - start_time)


if __name__ == "__main__":
    for i in range(5):
        Runner().start()
