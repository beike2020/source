'''
@author: beike2020
'''

import sys
import struct
import random
import pickle
import driverlib
from immlib import *
from ctypes import *

class ioctl_hook( LogBpHook ):
    def __init__( self ):
        self.imm     = Debugger()
        self.logfile = "C:\ioctl_log.txt"        
        LogBpHook.__init__(self)

    def run( self, regs ):
        in_buf = ""
        # read the IOCTL code
        ioctl_code = self.imm.readLong( regs['ESP'] + 8 )        
        # read out the InBufferSize
        inbuffer_size = self.imm.readMemory( regs['ESP'] + 0x10, 4)
        inbuffer_size = struct.unpack( "<L", inbuffer_size )[0]
        # now we find the buffer in memory to mutate
        inbuffer_ptr  = self.imm.readMemory( regs['ESP'] + 0xC, 4)
        inbuffer_ptr  = int(struct.unpack("<L", inbuffer_ptr)[0])
        # grab the original buffer
        in_buffer      = str(self.imm.readMemory( inbuffer_ptr, inbuffer_size )).encode("HEX")
        mutated_buffer = self.mutate( inbuffer_size )
        # write the mutated buffer into memory
        self.imm.writeMemory( inbuffer_ptr, mutated_buffer )
        # save the test case to file
        self.save_test_case( ioctl_code, in_buffer, mutated_buffer )
                            
    def mutate( self, inbuffer_size ):
        counter        = 0
        mutated_buffer = ""
        # We are simply going to mutate the buffer with random bytes
        while counter < inbuffer_size:
            mutated_buffer += struct.pack("H", random.randint( 0, 255 ) )[0]
            counter += 1
        return mutated_buffer

    def save_test_case( self, ioctl_code, in_buffer, mutated_buffer ):
        message  = "*****\n"
        message += "IOCTL Code:      0x%08x\n" % ioctl_code
        message += "Original Buffer: %s\n" % in_buffer
        message += "Mutated Buffer:  %s\n" % mutated_buffer.encode("HEX")
        message += "*****\n\n"
        fd = open( self.logfile, "a")
        fd.write( message )
        fd.close()
 
        
def immdbg_driverfuzz(args):
    imm = Debugger()
    deviceiocontrol = imm.getAddress( "kernel32.DeviceIoControl" )
    ioctl_hooker = ioctl_hook()
    ioctl_hooker.add( "%08x" % deviceiocontrol, deviceiocontrol )
    return "[*] IOCTL Fuzzer Ready for Action!"


def driver_driverfuzz( args ):
    ioctl_list  = []
    device_list = []
    dbg    = Debugger()
    driver = driverlib.Driver()

    # Grab the list of IOCTL codes and device names
    ioctl_list  = driver.getIOCTLCodes()
    if not len(ioctl_list):
        return "[*] ERROR! Couldn't find any IOCTL codes."
    
    device_list = driver.getDeviceNames()
    if not len(device_list):
        return "[*] ERROR! Couldn't find any device names."
    
    # Now create a keyed dictionary and pickle it to a file
    master_list = {}
    master_list["ioctl_list"]  = ioctl_list
    master_list["device_list"] = device_list
    fd = open( args[0], "wb")
    pickle.dump( master_list, fd )
    fd.close()
    return "[*] SUCCESS! Saved IOCTL codes and device names to %s" % args[0]


def ioctl_driverfuzz(argv):
    kernel32 = windll.kernel32 
    # Defines for Win32 API Calls
    GENERIC_READ    = 0x80000000
    GENERIC_WRITE   = 0x40000000
    OPEN_EXISTING   = 0x3  
    # Open the pickle and retrieve the dictionary 
    fd          = open(sys.argv[1], "rb")
    master_list = pickle.load(fd)
    ioctl_list  = master_list["ioctl_list"]
    device_list = master_list["device_list"]
    fd.close()
   
    # Now test that we can retrieve valid handles to all device names, any that don't pass we remove from our test cases
    valid_devices = []   
    for device_name in device_list:   
        # Make sure the device is accessed properly
        device_file = u"\\\\.\\%s" % device_name.split("\\")[::-1][0]    
        print "[*] Testing for device: %s" % device_file   
        driver_handle = kernel32.CreateFileW(device_file,GENERIC_READ|GENERIC_WRITE,0,None,OPEN_EXISTING,0,None)
        if driver_handle:            
            print "[*] Success! %s is a valid device!"    
            if device_file not in valid_devices:
                valid_devices.append( device_file )           
            kernel32.CloseHandle( driver_handle )
        else:
            print "[*] Failed! %s NOT a valid device."
    
    if not len(valid_devices):
        print "[*] No valid devices found. Exiting..."
        sys.exit(0)
    
    # Now let's begin feeding the driver test cases until we can't bear it anymore! CTRL-C to exit the loop and stop fuzzing
    while 1:    
        # Open the log file first
        fd = open("my_ioctl_fuzzer.log","a")  
        # Pick a random device name
        current_device = valid_devices[ random.randint(0, len(valid_devices)-1 ) ]
        fd.write("[*] Fuzzing: %s" % current_device)      
        # Pick a random IOCTL code
        current_ioctl  = ioctl_list[ random.randint(0, len(ioctl_list)-1)]
        fd.write("[*] With IOCTL: 0x%08x" % current_ioctl)    
        # Choose a random length
        current_length = random.randint(0, 10000) y
        fd.write("[*] Buffer length: %d" % current_length)  
        # Let's test with a buffer of repeating A's Feel free to create your own test cases here
        in_buffer      = "A" * current_length   
        # Give the IOCTL run an out_buffer
        out_buf        = (c_char * current_length)()
        bytes_returned = c_ulong(current_length)  
        # Obtain a handle
        driver_handle = kernel32.CreateFileW(device_file, GENERIC_READ|GENERIC_WRITE,0,None,OPEN_EXISTING,0,None)  
        fd.write("!!FUZZ!!")
        # Run the test case
        kernel32.DeviceIoControl( driver_handle, current_ioctl, in_buffer, current_length, byref(out_buf), current_length, byref(bytes_returned), None )   
        fd.write( "[*] Test case finished. %d bytes returned.\n" % bytes_returned.value )       
        # Close the handle and carry on!
        kernel32.CloseHandle( driver_handle )
        fd.close()


def main( args ):
    immdbg_driverfuzz(args)
    driver_driverfuzz(args)
    ioctl_driverfuzz(args)