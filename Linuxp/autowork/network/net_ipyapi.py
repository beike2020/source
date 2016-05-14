#!/usr/bin/env python

from IPy import IP

ip_s = raw_input('Please input an IP or net-range: ')
ips = IP(ip_s)

print('iptype: %s' % ips.iptype())
print('ipversion: %s' % ips.version())
print('int ip: %s' % ips.int())
print('hex ip: %s' % ips.strHex())
print('bin ip: %s' % ips.strBin())

if len(ips) > 1:
    print('net: %s' % ips.net())
    print('netmask: %s' % ips.netmask())
    print('broadcast: %s' % ips.broadcast())
    print('reverse address: %s' % ips.reverseNames()[0])
    print('subnet len: %s' % ips.len())
    print('netscope: %s = %s' %(ips.strNormal(2), ips.strNormal(3)))
    for x in ips:
        print(x)
else:
    print('reverse address: %s' % ips.reverseNames()[0])

ip_1 = raw_input('Please input an IP or net-range to compare: ')
ip_2 = raw_input('Please input an IP or net-range to compare: ')
if IP(ip_1) < IP(ip_2):
    print ip_1 + ' < ' + ip_2
else:
    print ip_1 + ' >= ' + ip_2
if ip_1 in IP(ip_2):
    print ip_1 + ' in ' + ip_2
else:
    print ip_1 + ' not in ' + ip_2
if IP(ip_1).overlaps(ip_2):
    print ip_1 + ' overlaps ' + ip_2
else:
    print ip_1 + ' not overlaps ' + ip_2
