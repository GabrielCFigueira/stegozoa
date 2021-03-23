#include "stegozoa_hooks.h"
#include <stdio.h>

//Optimization idea: get 3 last bits instead of index % 8, as division is heavier


#define MASK 0xFE

#define rotate(byte, rotation) ((byte << rotation) | (byte >> (8 - rotation)))
#define getLsb(num) (num & 0x0001)
#define getBit(A, index) (getLsb(A[index / 8] >> (index % 8)))
#define setBit(A, index, bit) \
    (A[index / 8] = (A[index / 8] & rotate(MASK, index % 8)) | (bit << index % 8))

static unsigned char msg[] = "!Why are we still here... just to suffer? ";
static int msgBit = 0;

int writeQdctLsb(short *qcoeff, int has_y2_block) {

    int rate = 0;
    int n_bits = sizeof(msg) * 8;
    //future idea: loop unroll
    for(int i = 0; i < 384 + has_y2_block * 16; i++) {
        if(msgBit < n_bits && qcoeff[i] != 1 && qcoeff[i] != 0 && (!has_y2_block || i % 16 != 0 || i > 255)) {
            qcoeff[i] = (qcoeff[i] & 0xFFFE) | getBit(msg, msgBit);
            msgBit++;
            rate++;
        }
            
        if(msgBit == n_bits)
            msgBit = 0; //send the same message over and over, for now
    }

    static int stop = 0;
    if (stop++ < 1) {
        printf("MASK: %d\n", rotate(MASK, 0));
        printf("MASK: %d\n", rotate(MASK, 1));
        printf("MASK: %d\n", rotate(MASK, 2));
        printf("MASK: %d\n", rotate(MASK, 3));
        printf("MASK: %d\n", rotate(MASK, 4));
        printf("MASK: %d\n", rotate(MASK, 5));
        printf("MASK: %d\n", rotate(MASK, 6));
        printf("MASK: %d\n", rotate(MASK, 7));
    }

    return rate;
    
}

static unsigned char msgReceived[500]; //must be dynamic in the future
static int msgBitDec = 0;

void readQdctLsb(short *qcoeff, int has_y2_block) {

    for(int i = 0; i < 384 + has_y2_block * 16; i++) {
        if(qcoeff[i] != 1 && qcoeff[i] != 0 && (!has_y2_block || i % 16 != 0 || i > 255)) {
            setBit(msgReceived, msgBitDec, getLsb(qcoeff[i]));
            msgBitDec++;
        }

        if(msgBitDec > 7 && !msgReceived[msgBitDec / 8 - 1]) {
            printf("Message: %s\n", msgReceived);
            msgBitDec = 0;
        }

        else if (msgBitDec == 8 && msgReceived[0] != '!') {
            msgReceived[0] = msgReceived[0] >> 1;
            msgBitDec--;
        }

            
    }

}



void printQdct(short *qcoeff) {

    printf("\nMacroblock:");
    for(int i = 0; i < 400; i++) {
        if (i % 16 == 0)
            printf("\n");
        printf("%d,", qcoeff[i]);
    }
    printf("\n");

}




static int msgCharEnc = 0;
static int msgCharDec = 0;
unsigned char theMsg[400];

void writeQdct(short *qcoeff, char *eobs, int has_y2_block) {

    for(int i = 0; i < 384 + has_y2_block * 16; i++) {
        if((!has_y2_block || i % 16 != 0 || i > 255) && qcoeff[i] != 1 && qcoeff[i] != 0) {
            qcoeff[i] = msg[msgCharEnc++];

            if(msgCharEnc == sizeof(msg) - 1) {
                msgCharEnc = 0;
                return;
            }
        }
    }

    
}

void readQdct(short *qcoeff, int has_y2_block) {
    
    
    for(int i = 0; i < 384 + has_y2_block * 16; i++) {
        if((!has_y2_block || i % 16 != 0 || i > 255) && qcoeff[i] != 1 && qcoeff[i] != 0) {
            theMsg[msgCharDec++] = qcoeff[i];
            if(msgCharDec == 1 && theMsg[0] != '!') {
                printf("OhNO\n");
                msgCharDec = 0;
            }
            if(msgCharDec == sizeof(msg) - 1) {
                theMsg[msgCharDec] = '\0';
                printf("Message: %s\n", theMsg);
                msgCharDec = 0;
                return;
            }
        }
    }


}

