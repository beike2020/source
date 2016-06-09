'''
@author: beike2020
'''

import threading
import sys
import struct
import random
import time
import utils
from ctypes import *
from pydbg import *
from pydbg.defines import *


dangerous_functions_resolved = {} 
crash_encountered = False
instruction_count = 0 
dangerous_functions = {
    "strcpy" : "msvcrt.dll",
    "strncpy" : "msvcrt.dll",
    "sprintf" : "msvcrt.dll", "vsprintf": "msvcrt.dll"
}


def breakpoint_test():
    msvcrt = cdll.msvcrt
    counter = 0
    while 1:
        msvcrt.printf("Loop iteration %d!\n", counter)
        time.sleep(2)
        counter += 1


# This is our user defined callback function
def printf_randomizer(dbg):
    # Read in the value of the counter at ESP + 0x8 as a DWORD
    parameter_addr = dbg.context.Esp + 0x8
    counter = dbg.read_process_memory(parameter_addr, 4)    
    # When using read_process_memory, it returns a packed binary string, we must first unpack it before we can use it further
    counter = struct.unpack("L", counter)[0]
    print "Counter: %d" % int(counter)    
    # Generate a random number and pack it into binary format so that it is written correctly back into the process
    random_counter = random.randint(1, 100)
    random_counter = struct.pack("L", random_counter)[0]       
    # Now swap in our random number and resume the process
    dbg.write_process_memory(parameter_addr,random_counter)   
    return DBG_CONTINUE


def pydbg_bphandle(): 
    dbg = pydbg()
    pid = raw_input("Enter the printf_loop.py PID: ")
    dbg.attach(int(pid))
    printf_address = dbg.func_resolve("msvcrt", "printf")
    # Set the breakpoint with the printf_randomizer function defined as a callback
    dbg.bp_set(printf_address, description="printf_address", handler=printf_randomizer)
    dbg.run()


def overflow_test():
    msvcrt = cdll.msvcrt
    raw_input("Once the debugger is attached, press any key.")
    buffer = c_char_p("AAAAA")
    overflow = "A" * 100
    msvcrt.strcpy(buffer, overflow)


def check_accessv(dbg):   
    # We skip first-chance exceptions
    if dbg.dbg.u.Exception.dwFirstChance:
        return DBG_EXCEPTION_NOT_HANDLED
    
    crash_bin = utils.crash_binning.crash_binning()
    crash_bin.record_crash(dbg)
    print crash_bin.crash_synopsis()
    dbg.terminate_process()
    return DBG_EXCEPTION_NOT_HANDLED


def pydbg_violation(): 
    pid = raw_input("Enter the Process ID: ")
    dbg = pydbg()
    dbg.attach(int(pid))
    dbg.set_callback(EXCEPTION_ACCESS_VIOLATION, check_accessv)
    dbg.run()


class snapshotter(object):
    def init (self, exe_path):
        self.exe_path = exe_path
        self.pid = None
        self.dbg = None
        self.running = True
        # Start the debugger thread, and loop until it sets the PID of our target process
        pydbg_thread = threading.Thread(target=self.start_debugger) 
        pydbg_thread.setDaemon(0)
        pydbg_thread.start()
        while self.pid == None: 
            time.sleep(1)
        # We now have a PID and the target is running; let's get a second thread running to do the snapshots
        monitor_thread = threading.Thread(target=self.monitor_debugger) 
        monitor_thread.setDaemon(0)
        monitor_thread.start() 
        
    def monitor_debugger(self):
        while self.running == True:
            input = raw_input("Enter: 'snap','restore' or 'quit'") 
            input = input.lower().strip()
            if input == "quit":
                print "[*] Exiting the snapshotter." 
                self.running = False 
                self.dbg.terminate_process()
            elif input == "snap":
                print "[*] Suspending all threads." 
                self.dbg.suspend_all_threads()
                print "[*] Obtaining snapshot." 
                self.dbg.process_snapshot()
                print "[*] Resuming operation." 
                self.dbg.resume_all_threads()
            elif input == "restore":
                print "[*] Suspending all threads." 
                self.dbg.suspend_all_threads()
                print "[*] Restoring snapshot." 
                self.dbg.process_restore()
                print "[*] Resuming operation." 
                self.dbg.resume_all_threads()
                
    def start_debugger(self): 
        self.dbg = pydbg()
        pid = self.dbg.load(self.exe_path) 
        self.pid = self.dbg.pid
        self.dbg.run() 


def pydbg_snapshot():
    exe_path = "C:\\WINDOWS\\System32\\calc.exe" 
    snapshotter(exe_path)


def danger_handler(dbg):
    # We want to print out the contents of the stack; that's about it
    # Generally there are only going to be a few parameters, so we will
    # take everything from ESP to ESP+20, which should give us enough
    # information to determine if we own any of the data esp_offset = 0
    print "[*] Hit %s" % dangerous_functions_resolved[dbg.context.Eip]
    print "================================================================="
    while esp_offset <= 20:
        parameter = dbg.smart_dereference(dbg.context.Esp + esp_offset)
        print "[ESP + %d] => %s" % (esp_offset, parameter)
        esp_offset += 4
    print "=================================================================\n"
    dbg.suspend_all_threads() 
    dbg.process_snapshot() 
    dbg.resume_all_threads()
    return DBG_CONTINUE


def access_violation_handler(dbg): 
    global crash_encountered
    # Something bad happened, which means something good happened :) Let's handle the access violation and then restore the process back to the last dangerous function that was called 
    if dbg.dbg.u.Exception.dwFirstChance:
        return DBG_EXCEPTION_NOT_HANDLED
    
    crash_bin = utils.crash_binning.crash_binning() 
    crash_bin.record_crash(dbg)
    print crash_bin.crash_synopsis()
    if crash_encountered == False: 
        dbg.suspend_all_threads() 
        dbg.process_restore() 
        crash_encountered = True
        # We flag each thread to single step
        for thread_id in dbg.enumerate_threads():
            print "[*] Setting single step for thread: 0x%08x" % thread_id 
        h_thread = dbg.open_thread(thread_id)
        dbg.single_step(True, h_thread) 
        dbg.close_handle(h_thread)
    # Now resume execution, which will pass control to our single step handler 
    dbg.resume_all_threads() 
    return DBG_CONTINUE
    else:
        dbg.terminate_process()
    return DBG_EXCEPTION_NOT_HANDLED


def single_step_handler(dbg): 
    global instruction_count 
    global crash_encountered
    if crash_encountered:
        if instruction_count == MAX_INSTRUCTIONS: 
            dbg.single_step(False)
            return DBG_CONTINUE
        else:
            # Disassemble this instruction
            instruction = dbg.disasm(dbg.context.Eip)
            print "#%d\t0x%08x : %s" % (instruction_count,dbg.context.Eip, instruction)
            instruction_count += 1 
            dbg.single_step(True)
            return DBG_CONTINUE


def pydbg_valunfind():
    dbg = pydbg()
    pid = int(raw_input("Enter the PID you wish to monitor: "))
    dbg.attach(pid)
    # Track down all of the dangerous functions and set breakpoints 
    for func in dangerous_functions.keys():
        func_address = dbg.func_resolve( dangerous_functions[func],func )
        print "[*] Resolved breakpoint: %s -> 0x%08x" % ( func, func_address ) 
        dbg.bp_set( func_address, handler = danger_handler ) 
        dangerous_functions_resolved[func_address] = func
    dbg.set_callback( EXCEPTION_ACCESS_VIOLATION, access_violation_handler ) 
    dbg.set_callback( EXCEPTION_SINGLE_STEP, single_step_handler )
    dbg.run()


if __name__ == "__main__":
    pydbg_bphandle()
    pydbg_violation()
    pydbg_snapshot()
    pydbg_valunfind()