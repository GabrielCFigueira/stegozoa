import socket
import time

socketPath = "/tmp/stegozoa_client_socket"


client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
client.connect(socketPath)

total = 0    
for i in range(20):
    start = time.time()
    client.send(bytes("Hello", 'ascii'))
    message = client.recv(1024)
    end = time.time()
    print("RTT: " + str(end - start))
    total += end - start

print("Average RTT: " + str(total / 50))
