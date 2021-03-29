#include "stegozoa_hooks.h"
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

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

#define BUFFER_LEN 1000

static unsigned char encoderBuff[BUFFER_LEN];
static unsigned char decoderBuff[BUFFER_LEN];
static int msgBitEnc = 0;
static int msgEncSize = 0;
static int msgBitDec = 0;
static int msgDecSize = 0;

static int encoderFd;
static int decoderFd;
static int initialized = 0;

static void error(char *errorMsg, char *when) {
    fprintf(stderr, "Stegozoa hooks error: %s when: %s\n", errorMsg, when);
}


int initialize() {

    static int dontRepeat = 0;

    encoderFd = open(ENCODER_PIPE, O_RDONLY | O_NONBLOCK);
    if(encoderFd < 1) {
        if(!dontRepeat)
            error(strerror(errno), "Trying to open the encoder pipe for reading");
        dontRepeat = 1;
        return 1;
    }

    decoderFd = open(DECODER_PIPE, O_WRONLY | O_NONBLOCK);
    if(decoderFd < 1) {
        if(!dontRepeat)
            error(strerror(errno), "Trying to open the decoder pipe for writing");
        dontRepeat = 1;
        return 2;
    }

    dontRepeat = 0;
    initialized = 1;
    return 0;
}

int isInitialized() {
    return initialized;
}

//moves array counting from arbitray position (bitIndex / 8) to the start of it
static void moveToStart(unsigned char array[], int *bitIndex, int *size) {

    
    int n_bits = *size - DIVIDE8(*bitIndex);

    if(n_bits <= 400 && DIVIDE8(*bitIndex) >= 400) {
        memcpy(array, array + DIVIDE8(*bitIndex), n_bits * sizeof(char));

        *size = n_bits;

    } else {

        int i = 0;
        for(int j = DIVIDE8(*bitIndex); j < *size; ++j, ++i) {
            array[i] = array[j];
        }

        *size = i;
    }

    *bitIndex = MOD8(*bitIndex);
    
}

static void fetchData(int currentFrame) {

    static int oldFrame = -1;
    static int padded = 0;

    if(oldFrame != currentFrame) {
        if(padded && msgBitEnc == msgEncSize * 8)
            padded = 0;
        oldFrame = currentFrame;
    }

    if(padded)
        return;

    moveToStart(encoderBuff, &msgBitEnc, &msgEncSize);

    int read_bytes = read(encoderFd, encoderBuff + msgEncSize + 2,
           BUFFER_LEN - msgEncSize - 2); //reserve 2 bytes for the message length

    if(read_bytes == -1) {
        error(strerror(errno), "Trying to read from the encoder pipe");
        read_bytes = 0;
    }

    if(read_bytes == 0 && !padded) {
        encoderBuff[msgEncSize++] = '\0';
        encoderBuff[msgEncSize++] = '\0';
        padded = 1;
    }
    else if(read_bytes > 0) { //assumes read_bytes is less than 16384 (16 bits)
        encoderBuff[msgEncSize++] = read_bytes & 0xFF;
        encoderBuff[msgEncSize++] = (read_bytes >> 8) & 0xFF;
        msgEncSize += read_bytes;
    }

}


int writeQdctLsb(short *qcoeff, int has_y2_block, int currentFrame) {

    
    if(msgEncSize - DIVIDE8(msgBitEnc) < 400)
        fetchData(currentFrame);

    if(msgBitEnc == msgEncSize * 8)
        return -1;
    
    int oldBitEnc = msgBitEnc;

    //future idea: loop unroll
    for(int i = 0; i < 384 + has_y2_block * 16; i++) {
        if(qcoeff[i] != 1 && qcoeff[i] != 0 && (!has_y2_block || MOD16(i) != 0 || i > 255)) {
            qcoeff[i] = (qcoeff[i] & 0xFFFE) | getBit(encoderBuff, msgBitEnc);
            msgBitEnc++;
            
            if(msgBitEnc == msgEncSize * 8)
                break;
        } 
    }

    return msgBitEnc - oldBitEnc;
    
}

static int parseHeader(unsigned char array[], int index) {
    int res = 0;

    res = (res | array[index + 1]) << 8;
    res = res | array[index];

    return res;
}

static int flushDecoder(int start) {

    int n_bytes;
    n_bytes = write(decoderFd, decoderBuff + start, DIVIDE8(msgBitDec) - start);
    msgBitDec = MOD8(msgBitDec);

    if(n_bytes == -1) {
        error(strerror(errno), "Trying to write to the decoder pipe");
        return 1;
    } else if(n_bytes < DIVIDE8(msgBitDec) - start) {
        error(strerror(errno), "Trying to write to the decoder pipe, wrote less bytes than expected");
        return 1;
    }

    return 0;
}

int readQdctLsb(short *qcoeff, int has_y2_block) {

    for(int i = 0; i < 384 + has_y2_block * 16; i++) {
        if(qcoeff[i] != 1 && qcoeff[i] != 0 && (!has_y2_block || MOD16(i) != 0 || i > 255)) {
            setBit(decoderBuff, msgBitDec, getLsb(qcoeff[i]));
            msgBitDec++;

            if(msgBitDec == 16) {
                msgDecSize = parseHeader(decoderBuff, 0) + 2;
                if (msgDecSize == 2) //padding indicating the end of the message in this frame
                    return 1;
            }
            else if(msgBitDec == msgDecSize * 8 && msgBitDec > 16) {
                if(flushDecoder(2))
                    return 1;
            }
        }
            
    }
    return 0;

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
