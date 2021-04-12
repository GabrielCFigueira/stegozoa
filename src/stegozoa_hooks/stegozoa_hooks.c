#include "stegozoa_hooks.h"
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>


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

static encoder_t* enc;
static encoder_t* lastEnc;
static decoder_t* dec;

static int encoderFd;
static int decoderFd;
static int embbedInitialized = 0;
static int extractInitialized = 0;

static void error(char *errorMsg, char *when) {
    fprintf(stderr, "Stegozoa hooks error: %s when: %s\n", errorMsg, when);
}

encoder_t *newEncoder() {
    encoder_t *encoder = (encoder_t *) malloc(sizeof(encoder_t));
    if(encoder == NULL)
        error("null pointer", "allocating new encoder_t");

    encoder->bit = 0;
    encoder->size = 0;
    encoder->ssrc = 0;
    encoder->next = NULL;
    
    return encoder;
}

decoder_t *newDecoder() {
    decoder_t *decoder = (decoder_t *) malloc(sizeof(decoder_t));
    if(decoder == NULL)
        error("null pointer", "allocating new decoder_t");
    
    decoder->bit = 0;
    decoder->size = 0;
    decoder->ssrc = 0;
    decoder->next = NULL;
    
    return decoder;
}

void releaseEncoder(encoder_t *encoder) {
    free(encoder);
}

void releaseDecoder(decoder_t *decoder) {
    free(decoder);
}

int initializeExtract() {


    static int dontRepeat = 0;

    decoderFd = open(DECODER_PIPE, O_WRONLY | O_NONBLOCK);
    if(decoderFd < 1) {
        if(!dontRepeat)
            error(strerror(errno), "Trying to open the decoder pipe for writing");
        dontRepeat = 1;
        return 1;
    }

    if(!(dec = newDecoder()))
        return 1;

    dontRepeat = 0;
    extractInitialized = 1;

    return 0;
}

int initializeEmbbed() {


    static int dontRepeat = 0;

    encoderFd = open(ENCODER_PIPE, O_RDONLY | O_NONBLOCK);
    if(encoderFd < 1) {
        if(!dontRepeat)
            error(strerror(errno), "Trying to open the encoder pipe for reading");
        dontRepeat = 1;
        return 1;
    }

    if(!(enc = newEncoder()))
        return 1;
    lastEnc = enc;

    dontRepeat = 0;
    embbedInitialized = 1;
    return 0;
}

int isEmbbedInitialized() {
    return embbedInitialized;
}

int isExtractInitialized() {
    return extractInitialized;
}

static int parseHeader(unsigned char array[], int index) {
    int res = 0;

    res = (res | array[index + 1]) << 8;
    res = res | array[index];

    return res;
}

void fetchData() {

    while (1) {

        unsigned char header[2];
        int read_bytes = read(encoderFd, header, 2);

        if(read_bytes != 2) {
            if(errno != EAGAIN) // read would block, no need to show this error
                error(strerror(errno), "Trying to read from the encoder pipe");
            read_bytes = 0;
        }

        int finished = (enc->bit == enc->size * 8);

        if(finished) {

            if(enc->next != NULL) {
                encoder_t *temp = enc;
                enc = enc->next;
                releaseEncoder(temp);
            } else {
                releaseEncoder(enc);
                enc = NULL;
            }

        }

        if(read_bytes == 0 && finished) {
            
            if(enc == NULL) {
                enc = newEncoder();
                lastEnc = enc;
                enc->buffer[0] = '\0';
                enc->buffer[1] = '\0';
                enc->size = 2;
            }

        } else if (read_bytes > 0) {

            encoder_t *newEnc = newEncoder();

            newEnc->buffer[0] = header[0];
            newEnc->buffer[1] = header[1];
           
            if(enc == NULL) {
                enc = newEnc;
                lastEnc = enc;
            } else {
                lastEnc->next = newEnc;
                lastEnc = newEnc;
            }

            read_bytes = read(encoderFd, newEnc->buffer + 2, parseHeader(header, 0));
        
            if(read_bytes != parseHeader(header, 0))
                error(strerror(errno), "Trying to read from the encoder pipe after reading the header!");

            else {
                newEnc->size = read_bytes + 2;
                printf("Consegui ler %d bytes\n", read_bytes);
            }
        }

        if(read_bytes == 0)
            break;
    
    }

}


int writeQdctLsb(short *qcoeff, int has_y2_block) {

    
    if(enc->bit == enc->size * 8)
        return -1;
    
    int oldBitEnc = enc->bit;

    //future idea: loop unroll
    for(int i = 0; i < 384 + has_y2_block * 16; i++) {
        if(qcoeff[i] != 1 && qcoeff[i] != 0 && (!has_y2_block || MOD16(i) != 0 || i > 255)) {
            qcoeff[i] = (qcoeff[i] & 0xFFFE) | getBit(enc->buffer, enc->bit);
            enc->bit++;
            
            if(enc->bit == enc->size * 8)
                break;
        } 
    }

    return enc->bit - oldBitEnc;
    
}

static int flushDecoder(int start) {

    int n_bytes;
    dec->bit = 0; //should be 0

    n_bytes = write(decoderFd, dec->buffer + start, dec->size - start);

    if(n_bytes == -1) {
        error(strerror(errno), "Trying to write to the decoder pipe");
        return 1;
    }
    else if(n_bytes < dec->size - start) {
        error(strerror(errno), "Trying to write to the decoder pipe, wrote less bytes than expected");
        return 1;
    }

    return 0;
}

int readQdctLsb(short *qcoeff, int has_y2_block) {

    //optimization idea: loop unroll
    for(int i = 0; i < 384 + has_y2_block * 16; i++) {
        if(qcoeff[i] != 1 && qcoeff[i] != 0 && (!has_y2_block || MOD16(i) != 0 || i > 255)) {
            setBit(dec->buffer, dec->bit, getLsb(qcoeff[i]));
            dec->bit++;

            if(dec->bit == 16) {
                dec->size = parseHeader(dec->buffer, 0) + 2;
                fprintf(stdout, "Header size: %d\n", dec->size);
                fflush(stdout);
                if (dec->size == 2) { //padding indicating the end of the message in this frame
                    dec->bit = 0;
                    return 1;
                }
            }
            else if(dec->bit == dec->size * 8 && dec->bit > 16) {
                if(flushDecoder(0))
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
