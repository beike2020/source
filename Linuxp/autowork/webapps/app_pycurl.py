# -*- coding: utf-8 -*-
import os
import sys
import time
import pycurl

def python_curl(url):
    c = pycurl.Curl()
    c.setopt(pycurl.URL, url)
    c.setopt(pycurl.CONNECTTIMEOUT, 5)
    c.setopt(pycurl.TIMEOUT, 5)
    c.setopt(pycurl.FORBID_REUSE, 1)
    c.setopt(pycurl.MAXREDIRS, 1)
    c.setopt(pycurl.NOPROGRESS, 1)
    c.setopt(pycurl.DNS_CACHE_TIMEOUT, 30)
    infile = open(os.path.dirname(os.path.realpath(__file__))+"/response.txt", "wb")
    c.setopt(pycurl.WRITEHEADER, infile)
    c.setopt(pycurl.WRITEDATA, infile)

    try:
        c.perform()
    except Exception,e:
        print "connecion error:"+str(e)
        infile.close()
        c.close()
        sys.exit()

    print "HTTP状态码：%s" %(c.getinfo(c.HTTP_CODE))
    print "DNS解析时间：%.2f ms" %(c.getinfo(c.NAMELOOKUP_TIME)*1000)
    print "建立连接时间：%.2f ms" %(c.getinfo(c.CONNECT_TIME)*1000)
    print "准备传输时间：%.2f ms" %(c.getinfo(c.PRETRANSFER_TIME)*1000)
    print "传输开始时间：%.2f ms" %(c.getinfo(c.STARTTRANSFER_TIME)*1000)
    print "传输结束总时间：%.2f ms" %(c.getinfo(c.TOTAL_TIME)*1000)
    print "下载数据包大小：%d bytes/s" %(c.getinfo(c.SIZE_DOWNLOAD))
    print "HTTP头部大小：%d bytes" %(c.getinfo(c.HEADER_SIZE))
    print "平均下载速度：%d bytes/s" %(c.getinfo(c.SPEED_DOWNLOAD))

    infile.close()
    c.close()

if __name__ == '__main__':
    python_curl("www.baidu.com")
