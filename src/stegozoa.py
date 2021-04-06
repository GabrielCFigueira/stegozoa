import os
import time
import threading
import queue

global encoderPipe, decoderPipe
decoderPipePath = "/tmp/stegozoa_decoder_pipe"
encoderPipePath = "/tmp/stegozoa_encoder_pipe"

established = False

messageQueue = queue.Queue()


def parseHooksHeader(header): #header: string with two chars
    size = int(header[0]) + (int(header[1]) << 8)
    return size


def receiveMessage():
    global messageQueue, established, decoderPipe
    while True:

        header = decoderPipe.read(2) #size header
        size = parseHooksHeader(header)
        
        body = decoderPipe.read(size) #message body
        msgType = int(body[0]) #message type

        message = body[1:] #payload
        print("Header size: " + str(size))
        
        if msgType == 0:
            pass #register new user

        elif not established:
            continue

        elif msgType == 1:
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



def createMessage(msgType, byteArray = bytes(0)):
    size = len(byteArray) + 1
    l1 = bytes([size & 0xff])
    l2 = bytes([(size & 0xff00) >> 8])
    return l1 + l2 + bytes([msgType]) + byteArray


def connect():
    global established, encoderPipe, decoderPipe

    if established:
        print("Connection is already established")
        return
    else:
        initialize()

    msgType = 0
    message = createMessage(msgType)

    encoderPipe.write(message)
    encoderPipe.flush()

def send(byteArray):
    global established, encoderPipe
    if not established:
        raise "Must establish connection first"
    
    #TODO validate packet size (cant be bigger than 10000?)
    message = createMessage(1, byteArray)

    encoderPipe.write(message)
    encoderPipe.flush()


def receive():
    global established, messageQueue
    if not established:
        raise "Must establish connection first"

    response = messageQueue.get()

    
    return response

if __name__ == "__main__":
    connect()
    message = "Why are we still here... just to suffer"
    send(bytes(message * 100, 'utf-8'))
    print(receive())

