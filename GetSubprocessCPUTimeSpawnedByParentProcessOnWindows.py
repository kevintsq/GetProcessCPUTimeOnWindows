import ctypes
import subprocess
import time
from threading import Thread

import pythoncom
import win32api
import win32con
import win32process
import wmi


class Runner(Thread):
    def run(self):
        pythoncom.CoInitialize()
        wmi_service = wmi.WMI()
        start_time = time.time()
        with subprocess.Popen(r"java -jar Test.jar") as process:
            process_info = wmi_service.query(
                f"SELECT * FROM Win32_Process WHERE ParentProcessId = {process.pid}")
            rtv = process.poll()
            while rtv is None:  # not finished
                process_info = wmi_service.query(
                    f"SELECT * FROM Win32_Process WHERE ParentProcessId = {process.pid}")
                rtv = process.poll()
            rtime = time.time() - start_time

            handle = process._handle
            creation_time = ctypes.c_ulonglong()
            exit_time = ctypes.c_ulonglong()
            kernel_time = ctypes.c_ulonglong()
            user_time = ctypes.c_ulonglong()
            rc = ctypes.windll.kernel32.GetProcessTimes(handle,
                                                        ctypes.byref(creation_time),
                                                        ctypes.byref(exit_time),
                                                        ctypes.byref(kernel_time),
                                                        ctypes.byref(user_time))
            if not rc:
                raise Exception('GetProcessTimes() returned an error')
            # result.rtime = (exit_time.value - creation_time.value) / 10000000
            ctime = (user_time.value + kernel_time.value) / 10000000

            if len(process_info) > 0:
                wmi_ctime = max((int(process_info[0].KernelModeTime) + int(process_info[0].UserModeTime)) / 10000000,
                                ctime)
                try:
                    handle = win32api.OpenProcess(win32con.PROCESS_ALL_ACCESS, False, process_info[0].ProcessId)
                    process_times = win32process.GetProcessTimes(handle)
                    ctime = max((process_times['KernelTime'] + process_times['UserTime']) / 10000000, wmi_ctime)
                    win32api.CloseHandle(handle)
                except:
                    ctime = wmi_ctime
            # pythoncom.CoUninitialize()
            print(f"return value: {rtv}")
            print(f"cputime: {ctime}")
            print(f"realtime: {rtime}")


if __name__ == "__main__":
    Runner().run()
