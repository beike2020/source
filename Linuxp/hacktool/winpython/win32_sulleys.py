'''
@author: beike2020
'''


from sulley import * 
from requests import ftp 


def sulley_ftp():
    s_initialize("user") 
    s_static("USER") 
    s_delim(" ") 
    s_string("justin") 
    s_static("\r\n") 
    s_initialize("pass") 
    s_static("PASS") 
    s_delim(" ") 
    s_string("justin") 
    s_static("\r\n") 
    s_initialize("cwd") 
    s_static("CWD") 
    s_delim(" ")
    s_string("c: ") 
    s_static("\r\n") 
    s_initialize("dele") 
    s_static("DELE") 
    s_delim(" ") 
    s_string("c:\\test.txt") 
    s_static("\r\n") 
    s_initialize("mdtm") 
    s_static("MDTM") 
    s_delim(" ") 
    s_string("C:\\boot.ini") 
    s_static("\r\n") 
    s_initialize("mkd") 
    s_static("MKD") 
    s_delim(" ") 
    s_string("C:\\TESTDIR") 
    s_static("\r\n")


def sulley_ftpbanner(sock):
    sock.recv(1024) 
    sess = sessions.session(session_filename="audits/warftpd.sess 
    target = sessions.target("192.168.244.133", 21) 
    target.netmon = pedrpc.client("192.168.244.133", 26001) 
    target.procmon = pedrpc.client("192.168.244.133", 26002)
    target.procmon_options = { "proc_name" : "war-ftpd.exe" }
    # Here we tie in the receive_ftp_banner function which receives a socket.socket() object from Sulley as its only parameter
    sess.pre_send = receive_ftp_banner sess.add_target(target) 
    sess.connect(s_get("user")) 
    sess.connect(s_get("user"), s_get("pass"))
    sess.connect(s_get("pass"), s_get("cwd")) 
    sess.connect(s_get("pass"), s_get("dele")) 
    sess.connect(s_get("pass"), s_get("mdtm")) 
    sess.connect(s_get("pass"), s_get("mkd")) 
    sess.fuzz()
    

if __name__ == "__main__":
    sulley_ftp()
    sulley_ftpbanner()