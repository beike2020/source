'''
@author: beike2020
'''

from idaapi import *


def idadbg_dangerfunc():
    danger_funcs = ["strcpy","sprintf","strncpy"]  
    for func in danger_funcs:    
        addr = LocByName( func )   
        if addr != BADADDR:    
            # Grab the cross-references to this address
            cross_refs = CodeRefsTo( addr, 0 )    
            print "Cross References to %s" % func
            print "-------------------------------"
            for ref in cross_refs:    
                print "%08x" % ref   
                # Color the call RED
                SetColor( ref, CIC_ITEM, 0x0000ff)    
            print


class FuncCoverage(DBG_Hooks):
    # Our breakpoint handler
    def dbg_bpt(self, tid, ea):
        print "[*] Hit: 0x%08x" % ea
        return 1


def idadbg_funref():
    # Add our function coverage debugger hook
    debugger = FuncCoverage()
    debugger.hook()   
    current_addr = ScreenEA()
    
    # Find all functions and add breakpoints
    for function in Functions(SegStart( current_addr ), SegEnd( current_addr )):
        AddBpt( function )
        SetBptAttr( function, BPTATTR_FLAGS, 0x0)  
    num_breakpoints = GetBptQty()  
    print "[*] Set %d breakpoints." % num_breakpoints
    

def idadbg_stacksize():
    var_size_threshold   = 16
    current_address      = ScreenEA()    
    for function in Functions(SegStart(current_address), SegEnd(current_address) ):    
        stack_frame   = GetFrame( function )    
        frame_counter = 0
        prev_count    = -1    
        frame_size    = GetStrucSize( stack_frame )    
        while frame_counter < frame_size:    
            stack_var = GetMemberName( stack_frame, frame_counter )    
            if stack_var != "":    
                if prev_count != -1:    
                    distance = frame_counter - prev_distance    
                    if distance >= var_size_threshold:
                        print "[*] Function: %s -> Stack Variable: %s (%d bytes)" % ( GetFunctionName(function), prev_member, distance )    
                else:    
                    prev_count    = frame_counter
                    prev_member   = stack_var    
                    try:
                        frame_counter = frame_counter + GetMemberSize(stack_frame, frame_counter)
                    except:
                        frame_counter += 1
            else:
                frame_counter += 1


if __name__ == "__main__":
    idadbg_dangerfunc()
    idadbg_funref()
    idadbg_stacksize()