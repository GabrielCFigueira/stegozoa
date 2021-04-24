import libstegozoa

import os
import socket
import threading
import sys

socketPath = "/tmp/stegozoa_client_socket"

server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)

server.bind(socketPath)
server.listen(1)

def send():
    while True:
        message = server.recv(10000)
        if message:
            libstegozoa.send(message, 255) #255 is the broadcast address

def receive():
    while True:
        message = libstegozoa.receive()
        server.send(message)

thread = threading.Thread(target=send, args=())
thread.start()
thread = threading.Thread(target=receive, args=())
thread.start()
libstegozoa.initialize()

if __name__ == "__main__":
    client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    client.connect(socketPath)
    if len(sys.argv) > 1:
        myId = int(sys.argv[1])
    else:
        myId = 1
    libstegozoa.connect(myId)
    while True:
        client.send(bytes("why are we still here... just to suffer?", ascii))
        print(client.recv(10000))
