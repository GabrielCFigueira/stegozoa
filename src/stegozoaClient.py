import libstegozoa

import os
import socket
import threading
import sys
import time

socketPath = "/tmp/stegozoa_client_socket"

def is_socket_closed(sock):
    try:
    # this will try to read bytes without blocking and also without removing them from buffer (peek only)
        data = sock.recv(16, socket.MSG_DONTWAIT | socket.MSG_PEEK)
        if len(data) == 0:
            return True
    except BlockingIOError:
        return False  # socket is open and reading from it would block
    except ConnectionResetError:
        return True  # socket was closed for some other reason
    except Exception as e:
        print("unexpected exception when checking if a socket is closed")
        return False
    return False

if len(sys.argv) > 1:
    myId = int(sys.argv[1])
else:
    myId = 1

server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)

mutex = threading.Lock()

server.bind(socketPath)
server.listen(1)


libstegozoa.initialize()
server_socket, address = server.accept()

def send():
    global server_socket, myId

    established = False
    while True:
        message = ''
        
        try:
            message = server_socket.recv(2048)
        except socket.error as e:
            mutex.acquire()
            if is_socket_closed(server_socket):
                server_socket.close()
                server_socket, address = server.accept()
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
            server_socket.sendall(message)
        except socket.error as e:
            mutex.acquire()
            if is_socket_closed(server_socket):
                server_socket.close()
                server_socket, address = server.accept()
            mutex.release()

        

thread = threading.Thread(target=send, args=())
thread.start()
thread = threading.Thread(target=receive, args=())
thread.start()
