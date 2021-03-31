import os
import time


def initialize():

    try:
        os.mkfifo(encoderPipePath)
    except Exception as oe: 
        raise ValueError(oe)

    try:
        os.mkfifo(decoderPipePath)
    except Exception as oe: 
        raise ValueError(oe)

    encoderPipe = open(encoderPipePath, 'w')
    decoderPipe = open(decoderPipePath, 'r')

def shutdown():
    os.remove(encoderPipePath)
    os.remove(decoderPipePath)


decoderPipePath = "/tmp/stegozoa_decoder_pipe"
encoderPipePath = "/tmp/stegozoa_encoder_pipe"

syn = 0
ack = 0
established = False

def createMessage(syn, ack, string = ''):
    return chr(syn) + chr(ack) + string

def parseHeader(header): #header: string with two chars
    size = ord(header[0]) + ord(header[1]) * 256
    return size


def connect():
    global syn, ack, established

    if established:
        print("Connection is already established\n")
        return
    else:
        initialize()

    syn = 1
    ack = 1
    message = createMessage(syn, ack)

    encoderPipe.write(message)
    encoderPipe.flush()


    response = decoderPipe.read(4) #hooks header + transport header
    if ord(response[0]) == 1 and ord(response[1]) == 1:
        print("Connection established\n")
        established = True
    else:
        print("Unexpected syn/ack, connection not established\n")


def send(string):
    global syn, ack, established
    if not established:
        raise "Must establish connection first"
    
    syn = (syn + 1) % 256
    #TODO packet fragmentation
    message = createMessage(syn, ack, string)

    encoderPipe.write(message)
    encoderPipe.flush()


def receive():
    global syn, ack, established
    if not established:
        raise "Must establish connection first"

    
    response = decoderPipe.read(2) #hooks header

    size = parseHeader(response)


    response = decoderPipe.read(size)

    # TODO validate ack
    ack = ord(response[0])
    
    return response[2:]

if __name__ == "__main__":
    connect()
    send("Hello")
    print(receive())

