import socket
import time

socketPath = "/tmp/stegozoa_client_socket"

client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
client.connect(socketPath)
    
client.send(bytes("World", 'ascii'))
message = client.recv(5)

while True:
    client.send(bytes("why are we still here... just to suffer?" * 100, 'ascii'))
