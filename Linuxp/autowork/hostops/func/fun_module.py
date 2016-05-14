#!/usr/bin/env python

import func_module

class Mymodule(func_module.FuncModule):
    version = "0.0.1"
    api_version = "0.0.1"
    description = "My module for func."

    def echo(self,vcount):
        command="/usr/bin/tail -n "+str(vcount)+" /var/log/messages"
        cmdref = sub_process.Popen(command, stdout=sub_process.PIPE, stderr=sub_process.PIPE, shell=True, close_fds=True) 
        data = cmdref.communicate()
        return (cmdref.returncode, data[0], data[1])
