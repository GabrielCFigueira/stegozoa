#include "stegozoa_hooks.h"
#include <stdio.h>

//Optimization idea: get 3 last bits instead of index % 8, as division is heavier


#define MASK 0xFE

#define rotate(byte, rotation) (byte << rotation) | (byte >> (8 - rotation))
#define getLsb(num) num & 0x1
#define getBit(A, index) (getLsb(A[index / 8] >> (index % 8)))
#define setBit(A, index, bit) \
    A[index / 8] = (A[index / 8] & rotate(MASK, index % 8)) | (bit << index % 8)

char msg[] = "!Boromir did nothing wrong";
static int msgBit = 0;
static int stop = 0;

int writeQdctLsb(short *qcoeff) {

    int lastMsgBit = msgBit;
    int n_bits = (sizeof(msg) + 1) * 8;
    //future idea: loop unroll
    for(int i = 0; i < 384 ; i=i+16) {
        if(msgBit < n_bits && (qcoeff[i] > 1 || qcoeff[i] < 0)) {
            qcoeff[i] = (qcoeff[i] & 0xFFFE) | getBit(msg, msgBit);
            msgBit++;
        }
            
    }

    int rate = msgBit - lastMsgBit;

    if(msgBit == n_bits)
        msgBit = 0; //send the same message over and over, for now

    return rate;
    
}

void printQdct(short *qcoeff) {

    if(stop < 920) {
        stop++;
    for(int i = 0; i < 400; i++) {
        if (i % 16 == 0)
            printf("\n");
        printf("%d,", qcoeff[i]);
    }
    }
}

static char msgReceived[200]; //must be dynamic in the future
static int msgBitDec = 0;

void readQdctLsb(short *qcoeff) {

    test();

    for(int i = 0; i < 384 ; i=i+16) {
        if(qcoeff[i] > 1 || qcoeff[i] < 0) {
            setBit(msgReceived, msgBitDec, getLsb(qcoeff[i]));
            msgBitDec++;
        }
        if(msgBitDec % 8 == 0 && msgBitDec / 8 > 1) {

            if(msgReceived[0] != '!') {
                msgBitDec = 0;
                printf("%c\n", msgReceived[0]);
                break;
            }

            if (msgReceived[msgBitDec / 8 - 1] == '\0') {
                printf("Message: %s\n", msgReceived);
                msgBitDec = 0;
                break;
            }
        }
            
    }

}


//test
void test() {
    short qc[400];
    unsigned char testMsg[] = "G";

    for(int i = 0; i < 384 ; i=i+16) {
        qc[i] = 21;
    }

    q[32] = 0;

    int testMsgBit = 0;
    int n_bits = (sizeof(testMsg)+1)*8;

    for(int i = 0; i < 384 ; i=i+16) {
        if(testMsgBit < n_bits && (qc[i] > 1 || qc[i] < 0)) {
            qc[i] = (qc[i] & 0xFFFE) | getBit(testMsg, testMsgBit);
            testMsgBit++;
        }
            
    }

    unsigned char testReceivedMsg[200];
    testMsgBit = 0;
    for(int i = 0; i < 384 ; i=i+16) {
        if(qc[i] > 1 || qc[i] < 0) {
            setBit(testReceivedMsg, testMsgBit, getLsb(qc[i]));
            testMsgBit;
        }

        if(testMsgBit % 8 == 0 && testMsgBit / 8 > 1) {
            if (testMsgReceived[testMsgBit / 8 - 1] == '\0') {
                printf("Message: %s\n", testMsgReceived);
                break;
            }
        }
    }
}
