# -*- coding: utf-8 -*-
import os
import sys
import time
import logging
import warnings
import subprocess
from scapy.all import traceroute

warnings.filterwarnings("ignore", category=DeprecationWarning)
logging.getLogger("scapy.runtime").setLevel(logging.ERROR)

domains = raw_input('Please input one or more IP/domain: ')
target =  domains.split(' ')
dport = [80]
if len(target) >= 1 and target[0] != '':
    res,unans = traceroute(target, dport=dport,retry=-2)
    res.graph(target="> net_tracert.svg")
    time.sleep(1)
    subprocess.Popen("/usr/bin/convert net_tracert.svg net_tracert.png", shell=True)
else:
    print "IP/domain number of errors,exit"
