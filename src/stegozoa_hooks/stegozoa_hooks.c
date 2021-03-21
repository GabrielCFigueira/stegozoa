#include "stegozoa_hooks.h"
#include <stdio.h>

//Optimization idea: get 3 last bits instead of index % 8, as division is heavier


#define MASK 0xFE

#define rotate(byte, rotation) ((byte << rotation) | (byte >> (8 - rotation)))
#define getLsb(num) (num & 0x0001)
#define getBit(A, index) (getLsb(A[index / 8] >> (index % 8)))
#define setBit(A, index, bit) \
    (A[index / 8] = (A[index / 8] & rotate(MASK, index % 8)) | (bit << index % 8))

unsigned char msg[] = "Boromir";
static int msgBit = 0;

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


void writeQdct(short *qcoeff, char *eob) {

    unsigned int i = 384
    for(; i < 400 && i < 384 + sizeof(msg) + 1; i++)
        qcoeff[i] = msg[i-384];    
    *eob = i - 384;
    
}

void readQdct(short *qcoeff) {
    
    unsigned char theMsg[400];
    for(unsigned int i = 384; i < 400 && i < 384 + sizeof(msg) + 1; i++)
        theMsg[i-384] = qcoeff[i];

    printf("Message: %s\n", theMsg);

} 


void printQdct(short *qcoeff, short *qcoeffBlock) {

    printf("Macroblock:");
    for(int i = 384; i < 400; i++) {
        if (i % 16 == 0)
            printf("\n");
        printf("%d,", qcoeff[i]);
    }
    printf("\n");
    printf("Block:\n"); 
    for(int i = 0; i < 16; i++) {
        printf("%d,", qcoeffBlock[i]);
    }

    printf("\n");

}

static unsigned char msgReceived[200]; //must be dynamic in the future
static int msgBitDec = 0;

void readQdctLsb(short *qcoeff) {

    for(int i = 0; i < 384 ; i=i+16) {
        if(qcoeff[i] > 1 || qcoeff[i] < 0) {
            setBit(msgReceived, msgBitDec, getLsb(qcoeff[i]));
            msgBitDec++;
        }
        if(msgBitDec / 8 > 1) {

            if (msgReceived[msgBitDec / 8 - 1] == '\0' && msgBitDec % 8 == 0) {
                printf("Message: %s\n", msgReceived);
                msgBitDec = 0;
                break;
            }
        }
        else if (msgBitDec == 8 && msgReceived[0] != '!') {
            /*msgReceived[0] = msgReceived[0] >> 1;
            msgBitDec--;*/
            msgBitDec = 0;
            break;
        
        }
            
    }

}


