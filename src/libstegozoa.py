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


messageToSend = {}
messageToReceive = {}

globalMutex = threading.Lock()


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


def processRetransmission(syn, retransmissions, mutex, message):
    while True:
       
        mutex.acquire()
        size = len(retransmissions)
        if syn in retransmissions:
            encoderPipe.write(message)
            encoderPipe.flush()
        else:
            mutex.release()
            return

        mutex.release()

        time.sleep(103 - 100 * (0.995 ** size))




class sendQueue:

    def __init__(self):
        self.queue = {}
        self.syn = 65500
        self.mutex = threading.Lock() 

    def addMessage(self, message):
        self.mutex.acquire()
        if len(self.queue) > 10000:
            del(self.queue[min(self.queue)])
        self.queue[self.syn] = message
        syn = self.syn & 0xffff
        self.syn += 1
        self.mutex.release()
        return syn

    def getMessage(self, syn):
        self.mutex.acquire()
        least = min(self.queue) // 65536
        most = max(self.queue) // 65536
        if self.queue.get(least * 65536 + syn):
            message = self.queue[least * 65536 + syn]
        elif self.queue.get(most * 65536 + syn):
            message = self.queue[most * 65536 + syn]
        else:
            message = bytes(0)
        self.mutex.release()
        return message


class recvQueue:

    def __init__(self):
        self.queue = {}
        self.syn = 65500
        self.retransmissions = {}
        self.duplicates = 0
        self.mutex = threading.Lock()

    def addMessage(self, message, sender, receiver, syn):
        global messageQueue
        
        self.mutex.acquire()
        print("Expected syn: " + str(self.syn))
        if syn > self.syn and abs(syn - self.syn) < 10000 or syn + 65536 - self.syn < 10000:
            self.queue[syn] = message

            if syn in self.retransmissions:
                del(self.retransmissions[syn])

            print("Retransmission!")
            
            if syn < self.syn: #wrap around 65536
                syn += 65536
            for i in range(self.syn, syn):
                
                actualSyn = i & 0xffff

                if actualSyn in self.retransmissions or actualSyn in self.queue:
                    continue
                else:
                    self.retransmissions[actualSyn] = actualSyn

                response = createMessage(3, receiver, sender, 0, create2byte(actualSyn), True)
                
                thread = threading.Thread(target=processRetransmission, args=(actualSyn, self.retransmissions, self.mutex, response))
                thread.start() #have single thread doing this? TODO


        elif syn == self.syn:

            if syn in self.retransmissions:
                del(self.retransmissions[syn])

            messageQueue.put(message)
            if syn in self.queue:
                del(self.queue[syn])
            self.syn = (self.syn + 1) & 0xffff

            first = list(filter(lambda x: x >= self.syn, self.queue.keys()))
            second = list(filter(lambda x: x < self.syn, self.queue.keys())) #in case of wrap around 65536

            for key in sorted(first):
                if key == self.syn:

                    messageQueue.put(self.queue[key])
                    del(self.queue[key])
                    self.syn = (self.syn + 1) & 0xffff

                else:
                    break
            
            for key in sorted(second):
                if key == self.syn:

                    messageQueue.put(self.queue[key])
                    del(self.queue[key])
                    self.syn = (self.syn + 1) & 0xffff

                else:
                    break

        else:
            self.duplicates += 1
            print("Duplicates: " + str(self.duplicates))

        self.mutex.release()




def retransmit(receiver, synBytes):
    global messageToSend, myId, encoderPipe

    syn = parse2byte(synBytes)
    print("Retransmission request! " + str(syn))

    message = messageToSend[receiver].getMessage(syn)
    message = createMessage(4, myId, receiver, syn, message, True)
    encoderPipe.write(message)
    encoderPipe.flush()




def receiveMessage():
    global messageToSend, established, decoderPipe, encoderPipe, peers, globalMutex

    success = 0
    insuccess = 0
    
    while True:

        header = decoderPipe.read(2) #size header
        size = parse2byte(header)
        
        body = decoderPipe.read(size) #message body
        msgType = body[0] #message type
        sender = body[1] #sender
        receiver = body[2] #receiver
        syn = parse2byte(body[3:5]) #syn

        print("Syn: " + str(syn))

        if msgType == 0: #type 0 messages dont need crc, they should be small enough

            globalMutex.acquire()
            if sender not in messageToSend:
                messageToSend[sender] = sendQueue()
            globalMutex.release()
            
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

        globalMutex.acquire()
        if sender not in messageToSend:
            messageToSend[sender] = sendQueue()
        if sender not in messageToReceive:
            messageToReceive[sender] = recvQueue()
        globalMutex.release()
        
    

        if msgType == 1:
            if receiver == myId and sender not in peers:
                peers += [sender]

        elif not established:
            continue

        elif msgType == 2 or msgType == 4:
            if receiver == myId or receiver == 255: #255 is the broadcast address
                messageToReceive[sender].addMessage(message, sender, receiver, syn)


        elif msgType == 3:
            if receiver == myId:
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
    global established, encoderPipe, messageToSend, globalMutex
    if not established:
        raise "Must establish connection first"
    
    #TODO validate packet size (cant be bigger than 10000?)
    if len(byteArray) > 16375: #header + payload <= 16384
        raise ValueError("message must be smaller or equal to 10000 bytes")

    globalMutex.acquire()
    if receiver not in messageToSend:
        messageToSend[receiver] = sendQueue()
    globalMutex.release()

    syn = messageToSend[receiver].addMessage(byteArray)

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

