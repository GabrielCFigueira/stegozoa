import socket
import time



socketPath = "/tmp/stegozoa_client_socket"


client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
client.connect(socketPath)
    

message = client.recv(5)
client.send(bytes("World", 'ascii'))

for i in range(50):
    message = client.recv(5)
    client.send(bytes("World", 'ascii'))
