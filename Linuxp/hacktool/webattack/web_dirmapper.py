#!/usr/bin/env python

import os
import Queue
import urllib2
import threading


threads   = 10 
target    = "http://www.test.com"
directory = "/opt/source/joomla-3.1.1"
filters   = [".jpg", ".gif", ".png", ".css"]


def test_remote(web_paths):
    while not web_paths.empty(): 
        path = web_paths.get()
        url = "%s%s" % (target, path)
        request = urllib2.Request(url) 
        try:
            response = urllib2.urlopen(request)
            content  = response.read()
            print "[%d] => %s" % (response.code, path) 
            response.close()
        except urllib2.HTTPError as error: 
            print "Failed %s" % error.code
            pass
        

if __name__ == '__main__':
    os.chdir(directory)
    web_paths = Queue.Queue() 
    for p, d, f in os.walk("."): 
        for files in f:
            remote_path = "%s/%s" % (p, files)
            if remote_path.startswith("."):
                remote_path = remote_path[1:]
            if os.path.splitext(files)[1] not in filters:
                web_paths.put(remote_path)

    for i in range(threads): 
        print "Spawning thread: %d" % i
        t = threading.Thread(target=test_remote, args=(web_paths,))
        t.start()
