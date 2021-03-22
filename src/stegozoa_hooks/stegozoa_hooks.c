#include "stegozoa_hooks.h"
#include <stdio.h>

//Optimization idea: get 3 last bits instead of index % 8, as division is heavier


#define MASK 0xFE

#define rotate(byte, rotation) ((byte << rotation) | (byte >> (8 - rotation)))
#define getLsb(num) (num & 0x0001)
#define getBit(A, index) (getLsb(A[index / 8] >> (index % 8)))
#define setBit(A, index, bit) \
    (A[index / 8] = (A[index / 8] & rotate(MASK, index % 8)) | (bit << index % 8))

unsigned char msg[] = "!Boromir did nothing wrong. Die frage ist nicht wo, die frage ist wann. What is going on here? Why is the message being cut randomly. It seems to have stopped. I always believed in the final victory. Anyway, how long is this message? It shouldn't be over 400 characters... for now i hope. Let's 400. Ok, let's make this easy: who is the biggest clown in the whole of Portugal? It's ";
static int msgBit = 0;

int writeQdctLsb(short *qcoeff, int has_y2_block) {

    int lastMsgBit = msgBit;
    int n_bits = (sizeof(msg) + 1) * 8;
    //future idea: loop unroll
    for(int i = 0; i < 384 + has_y2_block * 16 ; i++) {
        if(msgBit < n_bits && (qcoeff[i] > 1 || qcoeff[i] < 0) && (!has_y2_block || i % 16 != 0 || i > 255)) {
            qcoeff[i] = (qcoeff[i] & 0xFFFE) | getBit(msg, msgBit);
            msgBit++;
        }
            
    }

    int rate = msgBit - lastMsgBit;

    if(msgBit == n_bits)
        msgBit = 0; //send the same message over and over, for now

    return rate;
    
}

static unsigned char msgReceived[500]; //must be dynamic in the future
static int msgBitDec = 0;

void readQdctLsb(short *qcoeff, int has_y2_block) {

    for(int i = 0; i < 384 + has_y2_block * 16; i++) {
        if((qcoeff[i] > 1 || qcoeff[i] < 0) && (!has_y2_block || i % 16 != 0 || i > 255)) {
            setBit(msgReceived, msgBitDec, getLsb(qcoeff[i]));
            msgBitDec++;
        }
        if(msgBitDec / 8 > 1) {

            if (!msgReceived[msgBitDec / 8 - 1]) {
                printf("Message: %s\n", msgReceived);
                msgBitDec = 0;
                break;
            }
        }
        else if (msgReceived[0] != '!') {
            msgReceived[0] = msgReceived[0] >> 1;
            msgBitDec--;
            break;
        
        }
            
    }

}

static int msgCharEnc = 0;
static int msgCharDec = 0;
void writeQdct(short *qcoeff, char *eobs, int has_y2_block) {

    for(int i = 0; i < 384 + has_y2_block * 16; i++) {
        if((!has_y2_block || i % 16 != 0 || i > 255) && qcoeff[i] != 1 && qcoeff[i] != 0) {
            qcoeff[i] = msg[msgCharEnc++];

            //if(i % 16 == 15)
                //eobs[i / 16] = 16;
    
            if(msgCharEnc == sizeof(msg)) {
                //eobs[i / 16] = 16;
                msgCharEnc = 0;
                return;
            }
        }
    }

    
}

void readQdct(short *qcoeff, int has_y2_block) {
    
    unsigned char theMsg[400];
    
    for(int i = 0; i < 384 + has_y2_block * 16; i++) {
        if((!has_y2_block || i % 16 != 0 || i > 255) && qcoeff[i] != 1 && qcoeff[i] != 0) {
            theMsg[msgCharDec++] = qcoeff[i];
            printf("msgCharDec: %d\n", msgCharDec);
            if(msgCharDec == 1 && theMsg[0] != '!')
                msgCharDec = 0;
            else if(msgCharDec == 1)
                printQdct(qcoeff);
            if(msgCharDec == sizeof(msg)) {
                printf("Message: %s\n", theMsg);
                msgCharDec = 0;
                return;
            }
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

