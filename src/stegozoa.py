import os
import time
import threading
import queue
import sys

decoderPipePath = "/tmp/stegozoa_decoder_pipe"
encoderPipePath = "/tmp/stegozoa_encoder_pipe"
established = False
messageQueue = queue.Queue()
peers = []
myId = 255



def createMessage(msgType, sender, receiver, byteArray = bytes(0)):
    size = len(byteArray) + 3
    l1 = bytes([size & 0xff])
    l2 = bytes([(size & 0xff00) >> 8])
    return l1 + l2 + bytes([msgType]) + bytes([sender]) + bytes([receiver]) + byteArray

def parseHooksHeader(header): #header: string with two chars
    size = int(header[0]) + (int(header[1]) << 8)
    return size

def receiveMessage():
    global messageQueue, established, decoderPipe, peers
    while True:

        header = decoderPipe.read(2) #size header
        size = parseHooksHeader(header)
        
        body = decoderPipe.read(size) #message body
        msgType = body[0] #message type
        sender = body[1] #sender
        receiver = body[2] #receiver

        message = body[3:] #payload
        print("Header size: " + str(size))
        
        if msgType == 0:
            message = createMessage(1, myId, sender, message) #message is the ssrc in this case, must be sent back
            encoderPipe.write(message)
            encoderPipe.flush()

        elif msgType == 1:
            if receiver == myId:
                peers += [sender]

        elif not established:
            continue

        elif msgType == 2:
            if receiver == myId:
                messageQueue.put(message)
            


def initialize():

    global encoderPipe, decoderPipe, encoderPipePath, decoderPipePath
    try:
        os.mkfifo(encoderPipePath)
    except Exception as oe: 
        raise ValueError(oe)

    try:
        os.mkfifo(decoderPipePath)
    except Exception as oe: 
        raise ValueError(oe)

    encoderPipe = open(encoderPipePath, 'wb')
    decoderPipe = open(decoderPipePath, 'rb')

    thread = threading.Thread(target=receiveMessage, args=())
    thread.start()

def shutdown():
    global encoderPipePath, decoderPipePath
    os.remove(encoderPipePath)
    os.remove(decoderPipePath)


#---------------------API begins here---------------------------------

def connect(newId):
    global established, encoderPipe, decoderPipe, myId

    if established:
        print("Connection is already established")
        return
    #else:
    #    initialize()

    if not isinstance(newId, int) or newId < 0 or newId > 255:
        print("Invalid Id")
        return

    myId = newId

    msgType = 0
    message = createMessage(msgType, myId, 255) # 0xff = broadcast address

    encoderPipe.write(message)
    encoderPipe.flush()

    established = True

def send(byteArray, receiver):
    global established, encoderPipe
    if not established:
        raise "Must establish connection first"
    
    #TODO validate packet size (cant be bigger than 10000?)
    message = createMessage(2, myId, receiver, byteArray)

    encoderPipe.write(message)
    encoderPipe.flush()


def receive():
    global established, messageQueue
    if not established:
        raise "Must establish connection first"

    response = messageQueue.get()

    return response


def getPeers():
    global peers
    return peers



if __name__ == "__main__":
    if len(sys.argv) > 1:
        newId = int(sys.argv[1])
    else:
        newId = 1
    initialize()
    #connect(newId)
    #while len(getPeers()) < 1:
    #    time.sleep(0.5)
    #message = "Why are we still here... just to suffer"
    #send(bytes(message * 100, 'utf-8'), getPeers()[0])
    #print(receive())

