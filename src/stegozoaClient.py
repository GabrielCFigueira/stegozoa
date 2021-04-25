import libstegozoa

import os
import socket
import threading
import sys
import time

socketPath = "/tmp/stegozoa_client_socket"

if len(sys.argv) > 1:
    myId = int(sys.argv[1])
else:
    myId = 1

server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)

mutex = threading.Lock()

server.bind(socketPath)
server.listen(1)


server_socket, address = server.accept()

def send():
    global server_socket, myId

    established = False
    while True:
        message = ''
        
        try:
            message = server_socket.recv(10000)
        except socket.error as e:
            print("socket error" + str(server_socket.fileno()))
            mutex.acquire()
            if server_socket.fileno() == -1:
                server_socket.close()
                server_socket, address = server.accept()
            print("lets try to release the socket")
            mutex.release()
        
        if message:
            if not established:
                libstegozoa.connect(myId)
                established = True
            libstegozoa.send(message, 255) #255 is the broadcast address

def receive():
    global server_socket
    while True:
        message = libstegozoa.receive()

        try:
            server_socket.send(message)
        except socket.error as e:
            print("socket error" + str(server_socket.fileno()))
            mutex.acquire()
            if server_socket.fileno() == -1:
                server_socket.close()
                server_socket, address = server.accept()
            print("lets try to release the socket")
            mutex.release()

        

libstegozoa.initialize()
thread = threading.Thread(target=send, args=())
thread.start()
thread = threading.Thread(target=receive, args=())
thread.start()
