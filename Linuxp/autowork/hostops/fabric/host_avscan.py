#!/usr/bin/env python
# -*- coding: utf-8 -*-

import time
import pyclamd
from threading import Thread

class Scan(Thread):
    def __init__ (self, IP, scan_type, file):
        Thread.__init__(self)
        self.IP = IP
        self.scan_type = scan_type
        self.file = file
        self.connstr = ""
        self.scanresult = ""

    def run(self):
        try:
            cd = pyclamd.ClamdNetworkSocket(self.IP, 3310)
            fp = open('/tmp/EICAR', 'w').write(cd.EICAR())
            fp.close()
            if cd.ping():
                self.connstr = self.IP + " connection [OK]"
                cd.reload()
                if self.scan_type == "contscan_file":
                    self.scanresult = "{0}\n".format(cd.contscan_file(self.file))
                elif self.scan_type == "multiscan_file":
                    self.scanresult = "{0}\n".format(cd.multiscan_file(self.file))
                elif self.scan_type == "scan_file":
                    self.scanresult = "{0}\n".format(cd.scan_file(self.file))
                time.sleep(1)
            else:
                self.connstr = self.IP + " ping error,exit"
                return
        except Exception, e:
            self.connstr = self.IP +" " + str(e)

if __name__ == '__main__':
    i = 1
    scanlist = []
    IPs = ['10.1.80.85','192.168.1.22']

    for ip in IPs:
        currp = Scan(ip, "multiscan_file", "/tmp")
        scanlist.append(currp)

        if i % 10 == 0 or i == len(IPs):
            for task in scanlist:
                task.start()

            for task in scanlist:
                task.join()
                print task.connstr
                print task.scanresult
            scanlist = []   
        i += 1
