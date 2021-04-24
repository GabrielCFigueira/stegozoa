import stegozoaClient

import socket




socketPath = "/tmp/stegozoa_client_socket"


client = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
client.connect(socketPath)
if len(sys.argv) > 1:
    myId = int(sys.argv[1])
else:
    myId = 1

time.sleep(15)
while True:
    client.send(bytes("why are we still here... just to suffer?", 'ascii'))
    message, address = client.recvfrom(10000)
    print(message)
