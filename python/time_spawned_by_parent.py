"""GetSubprocessCPUTimeSpawnedByParentProcessOnWindows"""

import ctypes
import subprocess
import sys
from threading import Thread

import pythoncom
import win32api
import win32con
import win32process
import wmi


class Runner(Thread):
    def __init__(self, cmd):
        super().__init__()
        if not isinstance(cmd, str) and not isinstance(cmd, list) and not isinstance(cmd, tuple) or not cmd:
            raise Exception("cmd must be a non-empty string, a list or a tuple of strings")
        self.cmd = cmd

    def run(self):
        pythoncom.CoInitialize()
        wmi_service = wmi.WMI()
        with subprocess.Popen(self.cmd) as process:
            process_info = wmi_service.query(
                f"SELECT * FROM Win32_Process WHERE ParentProcessId = {process.pid}")
            rtv = process.poll()
            while rtv is None:  # not finished
                process_info = wmi_service.query(
                    f"SELECT * FROM Win32_Process WHERE ParentProcessId = {process.pid}")
                rtv = process.poll()

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
            rtime = (exit_time.value - creation_time.value) / 10000000
            stime = kernel_time.value / 10000000
            utime = user_time.value / 10000000
            ctime = (user_time.value + kernel_time.value) / 10000000

            if len(process_info) > 0:
                wmi_stime = max(int(process_info[0].KernelModeTime) / 10000000, stime)
                wmi_utime = max(int(process_info[0].UserModeTime) / 10000000, utime)
                wmi_ctime = max(wmi_stime + wmi_utime, ctime)
                try:
                    handle = win32api.OpenProcess(win32con.PROCESS_ALL_ACCESS, False, process_info[0].ProcessId)
                    process_times = win32process.GetProcessTimes(handle)
                    rtime = max((process_times['ExitTime'] - process_times['CreationTime']).total_seconds(), rtime)
                    stime = max(process_times['KernelTime'] / 10000000, wmi_stime)
                    utime = max(process_times['UserTime'] / 10000000, wmi_utime)
                    ctime = max(stime + utime, wmi_ctime)
                    win32api.CloseHandle(handle)
                except:
                    ctime = wmi_ctime
            # pythoncom.CoUninitialize()
            print(f"real\t{rtime}s")
            print(f"user\t{utime}s")
            print(f"sys\t{stime}s")
            print(f"cpu\t{ctime}s")
            print(f"Process finished with exit code{rtv}")


if __name__ == "__main__":
    Runner(sys.argv[1:]).run()
