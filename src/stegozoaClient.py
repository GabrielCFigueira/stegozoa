import libstegozoa

import os
import socket
import threading
import sys
import time
import signal

socketPath = "/tmp/stegozoa_client_socket"

def sigInt_handler(signum,frame):
    global socketPath
    os.remove(libstegozoa.encoderPipePath)
    os.remove(libstegozoa.decoderPipePath)
    os.remove(socketPath)
    exit(0)

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
    except socket.error:
        return True # socket error
    except Exception as e:
        print("unexpected exception when checking if a socket is closed")
        return False
    return False


def newConnection(server_socket):
    global mutex
    mutex.acquire()
    if is_socket_closed(server_socket):
        server_socket.close()
        server_socket, address = server.accept()
    mutex.release()


def send():
    global server_socket, myId

    established = False
    while True:
        message = ''
        
        try:
            message = server_socket.recv(4096)
        except socket.error as e:
            newConnection(server_socket)

        if message:
            if not established:
                libstegozoa.connect()
                established = True
            libstegozoa.send(message, 15) #15 is the broadcast address
        else:
            newConnection(server_socket)


def receive():
    global server_socket
    while True:
        message = libstegozoa.receive()

        try:
            server_socket.sendall(message)
        except socket.error as e:
            newConnection(server_socket)




signal.signal(signal.SIGINT,sigInt_handler)
mutex = threading.Lock()

if len(sys.argv) > 1:
    myId = int(sys.argv[1])
else:
    myId = 1

signal.signal(signal.SIGINT,libstegozoa.sigInt_handler)

server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)


server.bind(socketPath)
server.listen(1)


libstegozoa.initialize(myId)
server_socket, address = server.accept()

thread = threading.Thread(target=send, args=())
thread.start()
thread = threading.Thread(target=receive, args=())
thread.start()
