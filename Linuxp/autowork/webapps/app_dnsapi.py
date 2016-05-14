#!/usr/bin/python

import os
import httplib
import dns.resolver

iplist=[]
appdomain="baidu.com"


def get_iplist(domain=""):
    try:
        A = dns.resolver.query(domain, 'A')
    except Exception, e:
        print "DNS resolver error:" + str(e)
        return
    print "DNS resolver list: "
    for i in A.response.answer:
        for j in i.items:
            if j.rdtype == 1:
                print '\t' + j.address
                iplist.append(j.address) 
    return True


def get_cnlist(domain=""):
    try:
        CN = dns.resolver.query('www.'+domain, 'CNAME')
    except Exception, e:
        print "CN resolver error:" + str(e)
        return
    print "CN resolver list: "
    for i in CN.response.answer:
        for j in i.items:
            print '\t' + j.to_text()


def get_nslist(domain=""):
    try:
        NS = dns.resolver.query(domain, 'NS')
    except Exception, e:
        print "NS resolver error:" + str(e)
        return
    print "NS resolver list: "
    for i in NS.response.answer:
        for j in i.items:
            print '\t' + j.to_text()


def get_mxlist(domain=""):
    try:
        MX = dns.resolver.query(domain, 'MX')
    except Exception, e:
        print "MX resolver error:" + str(e)
        return
    print "MX resolver list: "
    for i in MX:
        print '\tMX preference =', i.preference, 'mail exchanger =', i.exchange


def checkip(ip):
    checkurl = ip + ':80'
    httplib.socket.setdefaulttimeout(5)
    conn = httplib.HTTPConnection(checkurl)

    try:
        getcontent = ""
        conn.request("GET", "/", headers = {"Host":appdomain})
        r = conn.getresponse()
        getcontent = r.read(15)
    finally:
        if getcontent == "<!doctype html>":
            print '\t' + ip + ' [OK]'
        else:
            print '\t' + ip + ' [Error]'


if __name__=="__main__":
    get_iplist(appdomain)
    get_nslist(appdomain)
    get_cnlist(appdomain)
    get_mxlist(appdomain)
    if len(iplist) > 0:
        print "IP check list: "
        for ip in iplist:
            checkip(ip)
