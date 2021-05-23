import socket
import time



socketPath = "/tmp/stegozoa_client_socket"


client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
client.connect(socketPath)
    

client.send(bytes("Hello", 'ascii'))
message = client.recv(5)

for i in range(50):
    start = time.time()
    client.send(bytes("Hello", 'ascii'))
    message = client.recv(5)
    end = time.time()
    print("RTT: " + str(end - start))
    total += end - start

print("Average RTT: " + str(total / 50))
