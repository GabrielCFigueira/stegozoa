import os
import time


global encoderPipe, decoderPipe
decoderPipePath = "/tmp/stegozoa_decoder_pipe"
encoderPipePath = "/tmp/stegozoa_encoder_pipe"

syn = 0
ack = 0
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



def createMessage(syn, ack, string = ''):
    return bytes(chr(syn) + chr(ack) + string, 'ascii')

def parseHeader(header): #header: string with two chars
    size = int(header[0]) + int(header[1]) * 256
    return size


def connect():
    global syn, ack, established, encoderPipe, decoderPipe

    if established:
        print("Connection is already established")
        return
    else:
        initialize()

    syn = 1
    ack = 1
    message = createMessage(syn, ack)

    print("connection Message: " + str(message))
    encoderPipe.write(message)
    encoderPipe.flush()


    response = decoderPipe.read(4) #hooks header + transport header
    if int(response[2]) == 1 and int(response[3]) == 1:
        print("Connection established")
        established = True
    else:
        print("Unexpected syn/ack, connection not established")
        print(response[2])
        print(response[3])


def send(string):
    global syn, ack, established, encoderPipe
    if not established:
        raise "Must establish connection first"
    
    syn = (syn + 1) % 256
    #TODO packet fragmentation
    message = createMessage(syn, ack, string)

    encoderPipe.write(message)
    encoderPipe.flush()


def receive():
    global syn, ack, established, decoderPipe
    if not established:
        raise "Must establish connection first"

    
    response = decoderPipe.read(2) #hooks header

    size = parseHeader(response)


    response = decoderPipe.read(size)

    # TODO validate ack
    ack = int(response[2])
    
    return response[2:]

if __name__ == "__main__":
    connect()
    send("Hello")
    print(receive())

