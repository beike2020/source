#!/usr/bin/env python

import re
import json
import base64
import socket
import urllib
from burp import IBurpExtender
from burp import IContextMenuFactory
from java.net import URL
from javax.swing import JMenuItem
from java.util import List, ArrayList

bing_api_key = "S53/kaUVJzLgAMVz2rme5xghEWhsw/Y2RFi+PeF2Llk"


class BurpExtender(IBurpExtender, IContextMenuFactory):
    def registerExtenderCallbacks(self, callbacks):
        self._callbacks = callbacks
        self._helpers   = callbacks.getHelpers()
        self.context    = None
        callbacks.setExtensionName("BHP Bing")
        callbacks.registerContextMenuFactory(self)
        return

    def createMenuItems(self, context_menu):
        self.context = context_menu
        menu_list = ArrayList()
        menu_list.add(JMenuItem("Send to Bing", actionPerformed=self.bing_menu))
        return menu_list

    def bing_menu(self,event):
        http_traffic = self.context.getSelectedMessages()
        print "%d requests highlighted" % len(http_traffic)
        for traffic in http_traffic:
            http_service = traffic.getHttpService()
            host         = http_service.getHost()
            print "User selected host: %s" % host
            self.bing_search(host)
        return

    def bing_search(self,host):
        # check if we have an IP or hostname
        is_ip = re.match("[0-9]+(?:\.[0-9]+){3}", host)
        if is_ip:
            ip_address = host
            domain     = False
        else:
            ip_address = socket.gethostbyname(host)
            domain     = True
        bing_query_string = "'ip:%s'" % ip_address
        self.bing_query(bing_query_string)
    
        if domain:
            bing_query_string = "'domain:%s'" % host
            self.bing_query(bing_query_string)

    def bing_query(self,bing_query_string): 
        print "Performing Bing search: %s" % bing_query_string
        quoted_query = urllib.quote(bing_query_string)
        http_request  = "https://api.datamarket.azure.com/Bing/Search/v1/Web?$format=json&$Query=%s HTTP/1.1\r\n" % quoted_query    
        http_request += "Host: api.datamarket.azure.com\r\n"
        http_request += "Connection: close\r\n"
        http_request += "Authorization: Basic %s\r\n" % base64.b64encode(":%s" % bing_api_key)
        http_request += "User-Agent: Blackhat Python\r\n\r\n"
        json_body = self._callbacks.makeHttpRequest("api.datamarket.azure.com",443,True,http_request).tostring()
        json_body = json_body.split("\r\n\r\n",1)[1]
        try:
            r = json.loads(json_body)
            if len(r["d"]["results"]):
                for site in r["d"]["results"]:
                    print "*" * 100
                    print site['Title']
                    print site['Url']
                    print site['Description']
                    print "*" * 100          
                    j_url = URL(site['Url'])
                    if not self._callbacks.isInScope(j_url):
                        print "Adding to Burp scope"
                        self._callbacks.includeInScope(j_url)
        except:
            print "No results from Bing"
            pass    
    
        return
