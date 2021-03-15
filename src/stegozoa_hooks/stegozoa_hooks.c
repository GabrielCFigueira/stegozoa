#include "stegozoa_hooks.h"
#include <stdio.h>

#define getBit(A, bit) ((A[bit / 8] >> (bit % 8)) & 0x1)

unsigned char msg[] = "Boromir did nothing wrong";
static int msgBit = 0;
static int stop = 0;

int writeQdctLsb(short *qcoeff) {

    int lastMsgBit = msgBit;
    int n_bits = (sizeof(msg) + 1) * 8;
    //future idea: loop unroll
    for(int i = 0; i < 384 ; i=i+16) {
        if(msgBit < n_bits && qcoeff[i]) {
            qcoeff[i] = (qcoeff[i] & 0xFFFE) | getBit(msg, msgBit);
            msgBit++;
            printf("%d\n", i);
        }
            
    }

    if(msgBit == n_bits)
        msgBit = 0; //send the same message over and over, for now

    return msgBit - lastMsgBit;
    
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

