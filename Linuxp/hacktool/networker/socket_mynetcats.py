#!/usr/bin/env python

import sys
import socket
import getopt
import threading
import subprocess


listen             = False
command            = False
upload             = False
execute            = ""
target             = ""
upload_destination = ""
port               = 0


def usage():
	print "Netcat Replacement"
	print
	print "Usage: socket_netcat.py -t target_host -p port"
	print "-l --listen                - listen on [host]:[port] for incoming connections"
	print "-e --execute=file_to_run   - execute the given file upon receiving a connection"
	print "-c --command               - initialize a command shell"
	print "-u --upload=destination    - upon receiving connection upload a file and write to [destination]"
	print
	print "Examples: "
	print "python socket_netcat.py -l -t 0.0.0.0 -p 5555 -c"
	print "python socket_netcat.py -l -t 0.0.0.0 -p 5555 -u /tmp/target.exe"
	print "python socket_netcat.py -l -t 0.0.0.0 -p 5555 -e \"cat /etc/passwd\""
	print "python socket_netcat.py -t localhost -p 5555 < echo 'ABCDEFGHI'"
	sys.exit(0)


def netcat_client():
	client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	try:
		client.connect((target, port))
		buffer = sys.stdin.read()
		if len(buffer):
			client.send(buffer)

		while True:
			recv_len = 1
			response = ""
			while recv_len:
				data = client.recv(4096)
				recv_len = len(data)
				response += data
				if recv_len < 4096:
					break
			print response 
				
			buffer = raw_input("")
			buffer += "\n"                        
			client.send(buffer)
			client.close()  
	except:
		print "[*] Exception! Exiting."
		client.close()  


def client_handle(client_socket):
	global upload
	global execute
	global command
	
	if len(upload_destination):
		file_buffer = ""
		while True:
			data = client_socket.recv(1024)
			if not data:
				break
			file_buffer += data
		try:
			file_descriptor = open(upload_destination, "wb")
			file_descriptor.write(file_buffer)
			file_descriptor.close()
			client_socket.send("Successfully saved file to %s\r\n" % upload_destination)
		except:
			client_socket.send("Failed to save file to %s\r\n" % upload_destination)
				
	if len(execute):
		cmds = execute.rstrip()
		try:
			output = subprocess.check_output(cmds, stderr=subprocess.STDOUT, shell=True)
		except:
			output = "Failed to execute command.\r\n"
		client_socket.send(output)

	if command:
		while True:
			client_socket.send("<Beike:#> ")
			cmd_buffer = ""
			while "\n" not in cmd_buffer:
				cmd_buffer += client_socket.recv(1024)
			cmds = cmd_buffer.rstrip()
			try:
				response = subprocess.check_output(cmds, stderr=subprocess.STDOUT, shell=True)
			except:
				response = "Failed to execute command.\r\n"
			client_socket.send(response)


def netcat_server():
	global target
	global port
	
	if not len(target):
		target = "0.0.0.0"
	server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	server.bind((target, port))
	server.listen(5)        
	while True:
		client_socket, addr = server.accept()
		client_thread = threading.Thread(target=client_handle, args=(client_socket,))
		client_thread.start()
			

if __name__ == '__main__':
	global listen
	global port
	global execute
	global command
	global upload_destination
	global target
	
	if not len(sys.argv[1:]):
		usage()
			
	try:
		opts, args = getopt.getopt(sys.argv[1:], "hle:t:p:cu:", ["help","listen","execute","target","port","command","upload"])
	except getopt.GetoptError as err:
		print str(err)
		usage()
			
	for o, a in opts:
		if o in ("-h","--help"):
			usage()
		elif o in ("-l","--listen"):
			listen = True
		elif o in ("-e", "--execute"):
			execute = a
		elif o in ("-c", "--commandshell"):
			command = True
		elif o in ("-u", "--upload"):
			upload_destination = a
		elif o in ("-t", "--target"):
			target = a
		elif o in ("-p", "--port"):
			port = int(a)
		else:
			assert False, "Unhandled Option"

	if listen:
		netcat_server()
	elif len(target) and port > 0:
		netcat_client()   
	
