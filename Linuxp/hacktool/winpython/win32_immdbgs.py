'''
@author: beike2020
'''

from immlib import *
import immutils 


"""Find custom code"""
def main(args):
    imm = Debugger()
    search_code = " ".join(args)
    search_bytes = imm.Assemble( search_code )    
    search_results = imm.Search( search_bytes )
    for hit in search_results:
        # Retrieve the memory page where this hit exists and make sure it's executable
        code_page   = imm.getMemoryPagebyAddress( hit )
        access      = code_page.getAccess( human = True )
        if "execute" in access.lower():
            imm.log("[*] Found: %s (0x%08x)" % ( search_code, hit ), address = hit )

    return "[*] Finished searching for instructions, check the Log window."


"""Find custom bar char"""
def immdbg_findbadchar(args):
    imm = Debugger()
    count = 0
    bad_char_found = False
    # First argument is the address to begin our search
    address = int(args[0],16)
    # Shellcode to verify
    shellcode = "<<COPY AND PASTE YOUR SHELLCODE HERE>>"   
    shellcode_length = len(shellcode)
    debug_shellcode = imm.readMemory( address, shellcode_length )
    debug_shellcode = debug_shellcode.encode("HEX")

    imm.log("Address: 0x%08x" % address)
    imm.log("Shellcode Length: %d" % shellcode_length)
    imm.log("Attack Shellcode: %s" % shellcode[:512])
    imm.log("In Memory Shellcode: %s" % debug_shellcode[:512])

    # Begin a byte-by-byte comparison of the two shellcode buffers
    while count <= shellcode_length:
        if debug_shellcode[count] != shellcode[count]:
            imm.log("Bad Char Detected at offset %d" % count)
            bad_char_found = True
            break
        count += 1
    if bad_char_found:
        imm.log("[*****] ")
        imm.log("Bad character found: %s" % debug_shellcode[count])
        imm.log("Bad character original: %s" % shellcode[count])
        imm.log("[*****] ")

    return "[*] !badchar finished, check Log window."


def tAddr(addr):
    buf = immutils.int2str32_swapped(addr)
    return "\\x%02x\\x%02x\\x%02x\\x%02x" % ( ord(buf[0]) , ord(buf[1]), ord(buf[2]), ord(buf[3]) )


"""Find address to bypass software DEP""" 
def main(args):
    imm = immlib.Debugger() 
    addylist = []
    mod = imm.getModule("ntdll.dll") 
    if not mod:
        return "Error: Ntdll.dll not found!"
    
    # Finding the First ADDRESS 
    ret = imm.searchCommands("MOV AL,1\nRET") 
    if not ret:
        return "Error: Sorry, the first addy cannot be found" 
    for a in ret:
        addylist.append( "0x%08x: %s" % (a[0], a[2]) )
    ret = imm.comboBox("Please, choose the First Address [sets AL to 1]", addylist)
    firstaddy = int(ret[0:10], 16)
    imm.Log("First Address: 0x%08x" % firstaddy, address = firstaddy)
    
    # Finding the Second ADDRESS ret = imm.searchCommandsOnModule( mod.getBase(), "CMP AL,0x1\n PUSH 0x2\n POP ESI\n" )
    if not ret:
        return "Error: Sorry, the second addy cannot be found" 
    secondaddy = ret[0][0]
    imm.Log( "Second Address %x" % secondaddy , address= secondaddy )
    
    # Finding the Third ADDRESS ret = imm.inputBox("Insert the Asm code to search for") 
    ret = imm.searchCommands(ret)
    if not ret:
        return "Error: Sorry, the third address cannot be found" 
    addylist = []
    for a in ret:
        addylist.append( "0x%08x: %s" % (a[0], a[2]) )
    ret = imm.comboBox("Please, choose the Third return Address [jumps to shellcode]", addylist)
    thirdaddy = int(ret[0:10], 16)
    imm.Log( "Third Address: 0x%08x" % thirdaddy, thirdaddy ) 
    imm.Log( 'stack = "%s\\xff\\xff\\xff\\xff%s\\xff\\xff\\xff\\xff" + "A" * 0x54 + "%s" + shellcode ' % ( tAddr(firstaddy), tAddr(secondaddy), tAddr(thirdaddy) ) )


"""Skip virus detect all debug exist by check IsDebuggerPresent""" 
def main(args):
    imm = immlib.Debugger() 
    imm.writeMemory( imm.getPEBaddress() + 0x2, "\x00" )
    

"""Skip virus detect all debug exist by enum all process""" 
def main(args):
    imm = immlib.Debugger() 
    process32first = imm.getAddress("kernel32.Process32FirstW") 
    process32next = imm.getAddress("kernel32.Process32NextW") 
    function_list = [ process32first, process32next ]
    patch_bytes = imm.Assemble( "SUB EAX, EAX\nRET" ) 
    for address in function_list:
        opcode = imm.disasmForward( address, nlines = 10 ) 
        imm.writeMemory( opcode.address, patch_bytes )
