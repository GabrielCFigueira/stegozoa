import os
import time
import threading
import queue
import sys
import signal

import crccheck

decoderPipePath = "/tmp/stegozoa_decoder_pipe"
encoderPipePath = "/tmp/stegozoa_encoder_pipe"
established = False
messageQueue = queue.Queue()
peers = []
myId = 255


def parseCRC(crc):
    res = int(crc[0])
    res += int(crc[1] << 8)
    res += int(crc[2] << 16)
    res += int(crc[3] << 24)
    return res

def createCRC(message):
    crc = crccheck.crc.Crc32.calc(message)
    l1 = bytes([crc & 0xff])
    l2 = bytes([(crc & 0xff00) >> 8])
    l3 = bytes([(crc & 0xff0000) >> 16])
    l4 = bytes([(crc & 0xff000000) >> 24])
    return l1 + l2 + l3 + l4

def validateCRC(message, crc):
    return createCRC(message) == crc


def parseSize(header): #header: string with two chars
    size = int(header[0]) + (int(header[1]) << 8)
    return size

def createSize(number):
    l1 = bytes([number & 0xff])
    l2 = bytes([(number & 0xff00) >> 8])
    return l1 + l2


def createMessage(msgType, sender, receiver, byteArray = bytes(0)):

    message = bytes([msgType]) + bytes([sender]) + bytes([receiver]) + byteArray
    size = createSize(len(message) + 4) # + 4 is the crc
    message = size + message
    message = message + createCRC(message)
    
    return message




def receiveMessage():
    global messageQueue, established, decoderPipe, peers
    while True:

        header = decoderPipe.read(2) #size header
        bodySize = parseSize(header)
        size = bodySize + 2 #full packet
        
        body = decoderPipe.read(bodySize) #message body
        msgType = body[0] #message type
        sender = body[1] #sender
        receiver = body[2] #receiver

        

        message = body[3:size - 4] #payload
        crc = body[size - 4:] #crc
        print("Header size: " + str(bodySize))

        if not validateCRC(header + body[:size - 4], parseCRC(crc)): 
            print("Corrupted message!")
            continue
        
        elif msgType == 0:
            message = createMessage(1, myId, sender, message) #message is the ssrc in this case, must be sent back
            encoderPipe.write(message)
            encoderPipe.flush()

        elif msgType == 1:
            if receiver == myId:
                peers += [sender]

        elif not established:
            continue

        elif msgType == 2:
            if receiver == myId or receiver == 255: #255 is the broadcast address
                messageQueue.put(message)

def sigInt_handler(signum,frame):
    global encoderPipePath, decoderPipePath
    os.remove(encoderPipePath)
    os.remove(decoderPipePath)
    exit(0)


#---------------------API begins here---------------------------------

def initialize():

    global encoderPipe, decoderPipe, encoderPipePath, decoderPipePath
    try:
        os.mkfifo(encoderPipePath)
    except Exception as oe: 
        raise

    try:
        os.mkfifo(decoderPipePath)
    except Exception as oe: 
        raise

    encoderPipe = open(encoderPipePath, 'wb')
    decoderPipe = open(decoderPipePath, 'rb')

    thread = threading.Thread(target=receiveMessage, args=())
    thread.start()

def shutdown():
    global encoderPipePath, decoderPipePath
    os.remove(encoderPipePath)
    os.remove(decoderPipePath)

def connect(newId = 255):
    global established, encoderPipe, decoderPipe, myId

    if established:
        print("Connection is already established")
        return

    if newId != 255:
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
    if len(byteArray) > 16375: #header + payload <= 16384
        raise ValueError("message must be smaller or equal to 10000 bytes")

    message = createMessage(2, myId, receiver, byteArray)

    encoderPipe.write(message)
    encoderPipe.flush()


def receive():
    global messageQueue
    return messageQueue.get()


def getPeers():
    global peers
    return peers



if __name__ == "__main__":
    if len(sys.argv) > 1:
        myId = int(sys.argv[1])
    else:
        myId = 1
    signal.signal(signal.SIGINT,sigInt_handler)
    initialize()
    connect(myId)
    #while len(getPeers()) < 1:
    #    time.sleep(0.5)
    message = "Why are we still here... just to suffer"
    #send(bytes(message, 'utf-8'), getPeers()[0])
    #print(receive())

