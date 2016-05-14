#!/usr/bin/env python

import os
import sys
import getopt
from scapy.all import *


def useage():
    print "useage: python socket_scapy.py -m[1-9] -s sip -d dip -i ethx -n number"

def ip_l4(dip, sip, ethx, num):
    send(IP(dst=dip, src=sip), iface=ethx, count=num)

def ip_version(dip, sip, ethx, num):
    send(IP(dst=dip, src=sip, version=0)/TCP(dport=80), iface=ethx, count=num)

def ip_sum(dip, sip, ethx, num):
    send(IP(dst=dip, src=sip, chksum=0x5500)/TCP(dport=80), iface=ethx, count=1000)
    send(IP(dst=dip, src=sip, frag=1)/TCP(dport=80), iface=ethx, count=1000)

def ip_length(dip, sip, ethx, num):
    send(IP(dst=dip, src=sip, ihl=5L, len=80)/TCP(dport=80), iface=ethx, count=num)

def ip_srcdst(dip, sip, ethx, num):
    send(IP(dst=dip, src=sip)/TCP(dport=80), iface=ethx, count=num)
    send(IP(dst=dip, src=dip)/TCP(dport=80), iface=ethx, count=num)

def icmp_flood(dip, sip, ethx, num):
    send(IP(dst=dip)/ICMP(), iface=ethx, count=num)

def tcp_ttl(dip, sip, ethx, num):
    send(IP(dst=dip, ttl=0)/TCP(), iface=ethx, count=num)

def tcp_sum(dip, sip, ethx, num):
    send(IP(dst=dip)/TCP(chksum=0x5555), iface=ethx, count=num) 

def tcp_flag(dip, sip, ethx, num):
    send(IP(dst=dip)/TCP(flags="F"), iface=ethx, count=num)
    send(IP(dst=dip)/TCP(flags="FS"), iface=ethx, count=num)
    send(IP(dst=dip)/TCP(flags=0x0ff), iface=ethx, count=num)
    send(IP(dst=dip)/TCP(flags="", seq=555), iface=ethx, count=num)

def tcp_length(dip, sip, ethx, num):
    send(IP(dst=dip, src=sip, ihl=2L)/TCP(dport=80), iface=ethx, count=num)
    send(IP(dst=dip, src=sip, ihl=15L)/TCP(dport=80), iface=ethx, count=num)
    send(IP(dst=dip, src=sip)/TCP(dport=80, dataofs=1L), iface=ethx, count=num)
    send(IP(dst=dip, src=sip)/TCP(dport=80, dataofs=15L), iface=ethx, count=num)


if __name__ == "__main__":
    if (len(sys.argv) < 10):
        useage()
        sys.exit(2)

    try:
        opts, args = getopt.getopt(sys.argv[1:], "hm:s:d:i:n:", ["help", "mode", "src", "dst", "iterface", "number"])
    except getopt.GetoptError:
        useage()
        sys.exit(2)

    for opt, arg in opts:
        if opt in ("-h", "--help"):
            usage()
            sys.exit()
        elif opt in ("-m", "--mode"):
            mode = arg
        elif opt in ("-s", "--src"):
            sip = arg
        elif opt in ("-d", "--dst"):
            dip = arg
        elif opt in ("-i", "--iterface"):
            ethx = arg
        elif opt in ("-n", "--number"):
            num = string.atoi(arg)

    if mode == "1":
        ip_l4(dip, sip, ethx, num)
        ip_version(dip, sip, ethx, num)
    elif mode == "2":
        ip_sum(dip, sip, ethx, num)
    elif mode == "3":
        ip_length(dip, sip, ethx, num)
    elif mode == "4":
        ip_srcdst(dip, sip, ethx, num)
    elif mode == "5":
        icmp_flood(dip, sip, ethx, num)
    elif mode == "6":
        tcp_ttl(dip, sip, ethx, num)
    elif mode == "7":
        tcp_sum(dip, sip, ethx, num)
    elif mode == "8":
        tcp_flag(dip, sip, ethx, num)
    elif mode == "9":
        tcp_length(dip, sip, ethx, num)
