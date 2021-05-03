import socket
import time



socketPath = "/tmp/stegozoa_client_socket"


client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
client.connect(socketPath)

start = time.time()
data = 0

for i in range(200):
    client.send(bytes("why are we still here... just to suffer?" * 200, 'ascii'))
    message = client.recv(10000)
    if message != bytes("why are we still here... just to suffer?" * 200, 'ascii'):
        print("Corrupted!")
    data += len(message)

end = time.time()
print("data: " + str(data) + " time: " + str(end - start))
print("Throughput: " + str(data / (end - start)))
