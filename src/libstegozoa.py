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

success = 0
insuccess = 0

syn = 0

messageToSend = {}
messageToReceive = {}


def createCRC(message):
    crc = crccheck.crc.Crc32.calc(message)
    l1 = bytes([crc & 0xff])
    l2 = bytes([(crc & 0xff00) >> 8])
    l3 = bytes([(crc & 0xff0000) >> 16])
    l4 = bytes([(crc & 0xff000000) >> 24])
    return l1 + l2 + l3 + l4

def validateCRC(message, crc):
    return createCRC(message) == crc


def parse2byte(header): #header: string with two chars
    size = int(header[0]) + (int(header[1]) << 8)
    return size

def create2byte(number):
    l1 = bytes([number & 0xff])
    l2 = bytes([(number & 0xff00) >> 8])
    return l1 + l2


def createMessage(msgType, sender, receiver, syn = 0, byteArray = bytes(0), crc = False):
    global messageToSend

    message = bytes([msgType]) + bytes([sender]) + bytes([receiver]) + create2byte(syn) + byteArray
    if crc:
        size = create2byte(len(message) + 4) # + 4 is the crc
        message = size + message
        message = message + createCRC(message)
    else:
        message = create2byte(len(message)) + message
    
    return message




class sendQueue:

    def __init__(self):
        self.queue = {}
        self.syn = 0

    def addMessage(self, message):
        if len(self.queue) > 1000:
            del(self.queue[min(self.queue)])
        self.queue[syn] = message
        self.syn += 1 #TODO mutex

    def getSyn(self):
        return self.syn & 0xffff # syn is 16 bits
            

class recvQueue:

    def __init__(self):
        self.queue = {}
        self.syn = 0

    def addMessage(self, message, sender, syn):
        global messageQueue, messageToSend

        if syn != self.syn:
            self.queue[syn] = message
            #possible retransmission needed?

            message = bytes(0)
            for i in range(self.syn, syn + 1): #TODO 65536 to 0
                message += create2byte(i)

            response = createMessage(3, myId, sender, messageToSend[receiver].getSyn(), message, True)
            encoderPipe.write(message)
            encoderPipe.flush()
        
        else:
            messageQueue.put(message)
            syn = (syn + 1) & 0xff
            for key in sorted(self.queue.keys()):
                if key == self.syn:
                    messageQueue.put(self.queue[key])
                    syn = (syn + 1) & 0xff
                else:
                    break




def retransmit(receiver, synArray):
    global messageToSend, myId

    synList = []
    for i in range(0, len(synArray), 2):
        synList += [parse2byte(synArray[i:i+2])]


    for syn in synList:
        message = messageToSend[receiver][syn]
        if message:
            message = createMessage(4, myId, receiver, syn, message, True)
            encoderPipe.write(message)
            encoderPipe.flush()




def receiveMessage():
    global messageToSend, established, decoderPipe, peers, success, insuccess
    while True:

        header = decoderPipe.read(2) #size header
        size = parse2byte(header)
        print("Header size: " + str(size))
        
        body = decoderPipe.read(size) #message body
        msgType = body[0] #message type
        sender = body[1] #sender
        receiver = body[2] #receiver
        msgSyn = parse2byte(body[3:5]) #syn

        print("Syn: " + str(msgSyn))

        if msgType == 0: #type 0 messages dont need crc, they should be small enough
            if receiver not in messageToSend:
                messageToSend[receiver] = sendQueue()
            
            message = createMessage(1, myId, sender, 0, body[5:size], True) #message is the ssrc in this case, must be sent back
            encoderPipe.write(message)
            encoderPipe.flush()
            continue
        

        message = body[5:size - 4] #payload
        crc = body[size - 4:] #crc

        success = success + 1
        
        if not validateCRC(header + body[:size - 4], crc): 
            print("Corrupted message!")
            insuccess = insuccess + 1
            success = success - 1
            continue


        if receiver not in messageToSend:
            messageToSend[receiver] = sendQueue()
        if sender not in messageToReceive:
            messageToReceive[sender] = recvQueue()
        
    

        if msgType == 1:
            if receiver == myId and sender not in peers:
                peers += [sender]

        elif not established:
            continue

        elif msgType == 2 or msgType == 4:
            if receiver == myId or receiver == 255: #255 is the broadcast address
                messageToReceive[sender].addMessage(message, receiver, syn)


        elif msgType == 3:
            retransmit(sender, message)


        print("Ratio: " + str(success * 1.0 / (success + insuccess)))



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
    global established, encoderPipe, myId

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
    global established, encoderPipe, messageToSend
    if not established:
        raise "Must establish connection first"
    
    #TODO validate packet size (cant be bigger than 10000?)
    if len(byteArray) > 16375: #header + payload <= 16384
        raise ValueError("message must be smaller or equal to 10000 bytes")

    if receiver not in messageToSend:
        messageToSend[receiver] = sendQueue()

    syn = messageToSend[receiver].getSyn()
    messageToSend[receiver].addMessage(byteArray)

    message = createMessage(2, myId, receiver, syn, byteArray, True)

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

