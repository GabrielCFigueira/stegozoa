import os
import time


global encoderPipe, decoderPipe
decoderPipePath = "/tmp/stegozoa_decoder_pipe"
encoderPipePath = "/tmp/stegozoa_encoder_pipe"

established = False


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

def shutdown():
    global encoderPipePath, decoderPipePath
    os.remove(encoderPipePath)
    os.remove(decoderPipePath)



def createMessage(msgType, byteArray = bytes(0)):
    size = len(byteArray)
    l1 = bytes([size & 0xff])
    l2 = bytes([(size & 0xff00) >> 8])
    return l1 + l2 + bytes([msgType]) + byteArray

def parseHooksHeader(header): #header: string with two chars
    size = int(header[0]) + (int(header[1]) << 8)
    return size


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


    response = decoderPipe.read(3) #hooks header + msgType header
    if int(response[2]) == 0:
        print("Connection established")
        established = True
    else:
        print("Unexpected message, connection not established")
        print(response)


def send(byteArray):
    global established, encoderPipe
    if not established:
        raise "Must establish connection first"
    
    #TODO validate packet size (cant be bigger than 10000?)
    message = createMessage(1, binArray)

    encoderPipe.write(message)
    encoderPipe.flush()


def receive():
    global established, decoderPipe
    if not established:
        raise "Must establish connection first"

    
    response = decoderPipe.read(2) #hooks header

    size = parseHooksHeader(response)

    print("Header size: " + str(size))


    response = decoderPipe.read(size)

    #TODO validate message type
    
    return response

if __name__ == "__main__":
    connect()
    message = "Why are we still here... just to suffer"
    send(bytes(message * 100, 'utf-8'))
    print(receive())

