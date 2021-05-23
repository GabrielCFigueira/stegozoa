import socket
import time



socketPath = "/tmp/stegozoa_client_socket"


client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
client.connect(socketPath)
    

client.send(bytes("World", 'ascii'))
message = client.recv(5)

for i in range(50):
    message = client.recv(5)
    client.send(bytes("World", 'ascii'))
