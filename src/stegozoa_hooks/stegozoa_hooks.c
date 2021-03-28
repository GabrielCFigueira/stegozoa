#include "stegozoa_hooks.h"
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define MASK 0xFE
#define DIVIDE8(num) (num >> 3)
#define MOD8(num) (num & 0x7)
#define MOD16(num) (num & 0xF)
#define getLsb(num) (num & 0x1)
#define rotate(byte, rotation) ((byte << rotation) | (byte >> (8 - rotation)))

#define getBit(A, index) (getLsb(A[index / 8] >> (MOD8(index))))
#define setBit(A, index, bit) \
    (A[DIVIDE8(index)] = (A[DIVIDE8(index)] & rotate(MASK, MOD8(index))) | (bit << MOD8(index)))

#define ENCODER_PIPE "/tmp/stegozoa_encoder_pipe"
#define DECODER_PIPE "/tmp/stegozoa_decoder_pipe"


static unsigned char msgSent[] = "!Why are we still here... just to suffer? ";
static int msgBitEnc = 0;
static unsigned char msgReceived[500];
static int msgBitDec = 0;

static int encoderFd;
static int decoderFd;
static int initialized = 0;

static void error(char *errorMsg, char *when) {
    fprintf(stderr, "Stegozoa hooks error: %s when: %s\n", errorMsg, when);
}


int initialize() {

    encoderFd = open(ENCODER_PIPE, O_RDONLY | O_NONBLOCK);
    if(encoderFd < 1) {
        error(strerror(errno), "Trying to open the encoder pipe for reading");
        return 1;
    }

    decoderFd = open(DECODER_PIPE, O_WRONLY | O_NONBLOCK);
    if(decoderFd < 1) {
        error(strerror(errno), "Trying to open the decoder pipe for writing");
        return 2;
    }

    initialized = 1;
    return 0;
}

int isInitialized() {
    return initialized;
}


int writeQdctLsb(short *qcoeff, int has_y2_block) {

    int rate = 0;
    int n_bits = sizeof(msgSent) * 8;
    //future idea: loop unroll
    for(int i = 0; i < 384 + has_y2_block * 16; i++) {
        if(qcoeff[i] != 1 && qcoeff[i] != 0 && (!has_y2_block || MOD16(i) != 0 || i > 255)) {
            qcoeff[i] = (qcoeff[i] & 0xFFFE) | getBit(msgSent, msgBitEnc);
            msgBitEnc++;
            rate++;
        
            
            if(msgBitEnc == n_bits)
                msgBitEnc = 0; //send the same message over and over, for now
        }
    }

    return rate;
    
}


void readQdctLsb(short *qcoeff, int has_y2_block) {

    for(int i = 0; i < 384 + has_y2_block * 16; i++) {
        if(qcoeff[i] != 1 && qcoeff[i] != 0 && (!has_y2_block || MOD16(i) != 0 || i > 255)) {
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
        if((!has_y2_block || MOD16(i) != 0 || i > 255) && qcoeff[i] != 1 && qcoeff[i] != 0) {
            qcoeff[i] = msgSent[msgCharEnc++];

            if(msgCharEnc == sizeof(msgSent) - 1) {
                msgCharEnc = 0;
                return;
            }
        }
    }

    
}

void readQdct(short *qcoeff, int has_y2_block) {
    
    
    for(int i = 0; i < 384 + has_y2_block * 16; i++) {
        if((!has_y2_block || MOD16(i) != 0 || i > 255) && qcoeff[i] != 1 && qcoeff[i] != 0) {
            theMsg[msgCharDec++] = qcoeff[i];
            if(msgCharDec == 1 && theMsg[0] != '!') {
                printf("OhNO\n");
                msgCharDec = 0;
            }
            if(msgCharDec == sizeof(msgSent) - 1) {
                theMsg[msgCharDec] = '\0';
                printf("Message: %s\n", theMsg);
                msgCharDec = 0;
                return;
            }
        }
    }


}

