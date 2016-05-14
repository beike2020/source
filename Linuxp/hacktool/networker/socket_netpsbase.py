#!/usr/bin/env python

import sys
import getopt
import socket
import threading


target_host = 'www.baidu.com'
target_port = 80
bind_host 	= '0.0.0.0'
bind_port 	= 80
is_server 	= 0
is_tcp 		= 0
is_udp 		= 0


def usage():
	print 'socket_base.py usage:'
	print '-h,--help'
	print '-t,--tcp'
	print '-u,--udp'
	print '-s,--server'
    print 'Examples: '
    print 'server: python socket_base.py -t -s'
    print 'client: python socket_base.py -t'
	sys.exit(0)


def tcp_client():
	client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	client.connect((bind_host, bind_port))
	client.send("GET / HTTP/1.1\r\nHost: www.baidu.com\r\n\r\n")
	response = client.recv(1024)
	print response


def udp_client():
	client = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	client.sendto("Hello, world!", ('127.0.0.1', target_port))
	response, addr = client.recvfrom(1024)
	print response 


def handle_tcp_client(client_socket):
	request = client_socket.recv(1024)
	print "[*] Received: %s" % request
	client_socket.send("ACK!")
	client_socket.close()


def tcp_server():
	server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	server.bind((bind_host, bind_port))
	server.listen(5)
	print "[*] Listening on %s:%d" %(bind_host, bind_port)
	while True:
		client, addr = server.accept()
		print "[*] Accepted connection from %s:%d" %(addr[0], addr[1])
		client_handler = threading.Thread(target=handle_tcp_client, args=(client,))
		client_handler.start()
	server.close()


def handle_udp_client(server_socket, request, client_addr):
	print "[*] Received: %s" % request
	server_socket.sendto('ACK!', (client_addr[0], client_addr[1]))	
	

def udp_server():
	server = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	server.bind((bind_host, bind_port))
	print "[*] Listening on %s:%d" %(bind_host, bind_port)
	while True:
		request, addr = server.recvfrom(1024)
		print "[*] Accepted connection from %s:%d" %(addr[0], addr[1])
		client_handler = threading.Thread(target=handle_udp_client, args=(server, request, addr,))
		client_handler.start()
	server.close()


if __name__ == '__main__':
	global is_server
	global is_tcp
	global is_udp

	if not len(sys.argv[1:]):
		usage()

	try:
		opts, args = getopt.getopt(sys.argv[1:], "htus", ['help', 'tcp', 'udp', 'server'])
	except getopt.GetoptError, err:
		print str(err)
		usage()

	for o, a in opts:
		if o in ('-h', '--help'):
			usage()
			sys.exit(-1)
		elif o in ('-s', 'server'):
			is_server = 1
		elif o in ('-u', 'udp'):
			is_udp = 1
		elif o in ('-t', 'tcp'):
			is_tcp = 1
		else:
			print 'unknow option'
			sys.exit(-1)
		
	if is_server == 1 and is_tcp == 1:
		tcp_server()
	elif is_server == 1 and is_udp == 1:
		udp_server()
	elif is_server == 0 and is_tcp == 1:
		tcp_client()
	elif is_server == 0 and is_udp == 1:
		udp_client()
	else:
		usage()
	
