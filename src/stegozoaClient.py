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

server.bind(socketPath)
server.listen(1)

established = False

server_socket, address = server.accept()

def send():
    while True:
        message = server_socket.recv(10000)
        if message:
            if not established:
                libstegozoa.connect(myId)
            libstegozoa.send(message, 255) #255 is the broadcast address

def receive():
    while True:
        message = libstegozoa.receive()
        server_socket.send(message)

thread = threading.Thread(target=send, args=())
thread.start()
thread = threading.Thread(target=receive, args=())
thread.start()
libstegozoa.initialize()
