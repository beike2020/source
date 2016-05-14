#!/usr/bin/env python
import os
import sys
import time
import paramiko


def paramiko_sshexce():
    hostname = '192.168.1.21'
    username = 'root'
    password = 'SKJh935yft#'

    paramiko.util.log_to_file('syslogin.log')

    ssh = paramiko.SSHClient()
    ssh.load_system_host_keys()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    #privatekey = os.path.expanduser('/home/key/id_rsa')
    #key = paramiko.RSAKey.from_private_key_file(privatekey)
    #ssh.connect(hostname=hostname,username=username,pkey = key)
    ssh.connect(hostname=hostname,username=username,password=password)
    stdin, stdout, stderr = ssh.exec_command('free -m')
    print stdout.read()
    ssh.close()


def paramiko_bastion(types):
    buff = ''
    resp = ''
    hostname = "192.168.1.21"
    username = "root"
    password = "SKJh935yft#"
    blip = "192.168.1.23"
    bluser = "root"
    blpasswd = "SKJh935yft#"
    localpath = "/etc/profile"
    tmppath = "/tmp/profile"
    remotepath = "/opt/profile"

    paramiko.util.log_to_file('syslogin.log')

    if (types == "scp"):
        t = paramiko.Transport((blip, 22))
        t.connect(username=bluser, password=blpasswd)
        sftp = paramiko.SFTPClient.from_transport(t)
        sftp.put(localpath, tmppath)
        sftp.close()

    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(hostname=blip, username=bluser, password=blpasswd)
    channel = ssh.invoke_shell()
    channel.settimeout(10)
    if (types == "ssh"):
        channel.send('ssh ' + username + '@' + hostname + '\n')
    eles:
        channel.send('scp ' + tmppath + ' ' + username + '@' + hostname + ':' + remotepath + '\n')

    while not buff.endswith('\'s password: '):
        try:
            resp = channel.recv(9999)
        except Exception, e:
            print 'Error info: %s connection time.' % (str(e))
            channel.close()
            ssh.close()
            sys.exit()
        buff += resp
        if not buff.find('yes/no') == -1:
            channel.send('yes\n')
	    buff = ''

    buff = ''
    channel.send(password + '\n')
    while not buff.endswith('# '):
        resp = channel.recv(9999)
        if not resp.find('\'s password: ') == -1:
            print 'Error info: Authentication failed.'
            channel.close()
            ssh.close()
            sys.exit() 
        buff += resp

    if (types == "ssh"):
        buff = ''
        channel.send('ifconfig\n')
        try: 
            while buff.find('# ') == -1:
                resp = channel.recv(9999)
                buff += resp
        except Exception, e:
            print "error info:" + str(e)

    print buff
    channel.close()
    ssh.close()


if __name__ == "":
    paramiko_sshexce()
    paramiko_bastion()
