#!/usr/bin/python

#python host_funapi.py fun_module.py 

import sys
import xmlrpclib
import func.overlord.client as fc

if __name__ == '__main__':
    module = sys.argv[1]
    pythonmodulepath = "/usr/lib/python2.6/site-packages/func/minion/modules/"
    client = fc.Client("*")
    fb = file(pythonmodulepath+module, "r").read()
    data = xmlrpclib.Binary(fb)
    print client.copyfile.copyfile(pythonmodulepath+module, data)
    print client.command.run("/etc/init.d/funcd restart")
