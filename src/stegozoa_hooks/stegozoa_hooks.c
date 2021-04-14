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

static context_t *encoders[256];
static int n_encoders = 0;
static context_t *decoders[256];
static int n_decoders = 0;

static unsigned char senderId;

static int encoderFd;
static int decoderFd;
static int embbedInitialized = 0;
static int extractInitialized = 0;

static void error(char *errorMsg, char *when) {
    fprintf(stderr, "Stegozoa hooks error: %s when: %s\n", errorMsg, when);
}

static message_t *newMessage() {
    message_t *message = (message_t *) calloc(1, sizeof(message_t));
    if(message == NULL)
        error("null pointer", "allocating new message_t");

    return message;
}

static context_t *newContext(uint32_t ssrc) {
    context_t *context = (context_t *) calloc(1, sizeof(context_t));
    if(context == NULL)
        error("null pointer", "allocating new context_t");
    
    context->ssrc = ssrc;
    context->msg = newMessage();
    return context;
}

static void releaseMessage(message_t *message) {
    free(message);
}

static int parseHeader(unsigned char array[], int index) {
    int res = 0;

    res = (res | array[index + 1]) << 8;
    res = res | array[index];

    return res;
}

static context_t *getEncoderContext(uint32_t ssrc) {

    for(int i = 0; i < n_encoders; i++)
        if(encoders[i]->ssrc == ssrc)
            return encoders[i];

    encoders[n_encoders++] = newContext(ssrc);
    return encoders[n_encoders - 1];
}

static context_t *getEncoderContextById(int id) {

    for(int i = 0; i < n_encoders; i++)
        for(int j = 0; j < encoders[i]->n_ids; j++)
            if(encoders[i]->id[j] == id)
                return encoders[i];

    return NULL;
}

static context_t *getDecoderContext(uint32_t ssrc) {

    for(int i = 0; i < n_decoders; i++)
        if(decoders[i]->ssrc == ssrc)
            return decoders[i];

    decoders[n_decoders++] = newContext(ssrc);
    return decoders[n_decoders - 1];
}

static void appendMessage(context_t *ctx, message_t *newMsg) {
    message_t *msg = ctx->msg;
    if(msg == NULL)
        ctx->msg = newMsg;
    else {
        while(msg->next != NULL) msg = msg->next;
        msg->next = newMsg;
    }
}

static message_t *copyMessage(message_t *msg) {
    message_t *newMsg = newMessage();
    newMsg->bit = msg->bit;
    newMsg->size = msg->size;
    memcpy(newMsg->buffer, msg->buffer, BUFFER_LEN * sizeof(unsigned char));
    return newMsg;
}

static void insertSsrc(message_t *msg, uint32_t ssrc) {
    int size = parseHeader(msg->buffer, 0) + 4; //32 bits = 4 bytes
    msg->size += 4;

    msg->buffer[0] = size & 0xff;
    msg->buffer[1] = (size >> 8) & 0xff;

    msg->buffer[5] = ssrc & 0xff;
    msg->buffer[6] = (ssrc >> 8) & 0xff;
    msg->buffer[7] = (ssrc >> 16) & 0xff;
    msg->buffer[8] = (ssrc >> 24) & 0xff;
}

static uint32_t obtainSsrc(message_t *msg) {
    uint32_t ssrc = 0;

    //probably unnecessary, but must make sure I can shift 24 bits correctly
    uint32_t ssrc1 = (uint32_t) msg->buffer[5];
    uint32_t ssrc2 = (uint32_t) msg->buffer[6];
    uint32_t ssrc3 = (uint32_t) msg->buffer[7];
    uint32_t ssrc4 = (uint32_t) msg->buffer[8];
    
    ssrc += ssrc1;
    ssrc += ssrc2 << 8;
    ssrc += ssrc3 << 16;
    ssrc += ssrc4 << 24;

    return ssrc;

}

void fetchData(uint32_t ssrc) {
    
    context_t *ctx = getEncoderContext(ssrc);
    message_t *msg = ctx->msg;


    while (1) { //remove this: may result in denial of service

        unsigned char header[2];
        int read_bytes = read(encoderFd, header, 2);

        if(read_bytes != 2) {
            if(errno != EAGAIN) // read would block, no need to show this error
                error(strerror(errno), "Trying to read from the encoder pipe");
            read_bytes = 0;
        }

        int finished = (msg->bit == msg->size * 8);

        if(finished) { //discard current message

            if(msg->next != NULL) {
                message_t *temp = msg;
                ctx->msg = msg->next;
                releaseMessage(temp);
            } else {
                releaseMessage(msg);
                ctx->msg = NULL;
            }

        }

        if(read_bytes == 0 && finished) {
            
            if(ctx->msg == NULL) {
                ctx->msg = newMessage();
                ctx->msg->buffer[0] = '\0';
                ctx->msg->buffer[1] = '\0';
                ctx->msg->size = 2;
            }

        } else if (read_bytes > 0) {

            message_t *newMsg = newMessage();

            newMsg->buffer[0] = header[0];
            newMsg->buffer[1] = header[1];
            
            read_bytes = read(encoderFd, newMsg->buffer + 2, parseHeader(header, 0));

            unsigned char msgType = newMsg->buffer[2];
            unsigned char sender = newMsg->buffer[3];
            unsigned char receiver = newMsg->buffer[4];

            if(read_bytes != parseHeader(header, 0)) {
                error(strerror(errno), "Trying to read from the encoder pipe after reading the header!");
                releaseMessage(newMsg);
            
            } else {
                newMsg->size = read_bytes + 2;
                printf("Consegui ler %d bytes\n", read_bytes);
                
                if(msgType == 0x0) {
                    senderId = sender;
                    insertSsrc(newMsg, ssrc);
                    for(int i = 0; i < n_encoders; ++i) {
                        appendMessage(encoders[i], newMsg);
                        newMsg = copyMessage(newMsg);
                    }
                    releaseMessage(newMsg);
                
                } else if(msgType == 0x1 || receiver == 0xff) {
                    insertSsrc(newMsg, ssrc);
                    for(int i = 0; i < n_encoders; ++i) {
                        appendMessage(encoders[i], newMsg);
                        newMsg = copyMessage(newMsg);
                    }
                    releaseMessage(newMsg);
                }
                else
                    appendMessage(getEncoderContextById((int) receiver), newMsg);

            }

        }

        if(read_bytes == 0)
            break;
    
    }

}


int writeQdctLsb(short *qcoeff, int has_y2_block, uint32_t ssrc) {

    message_t *msg = getEncoderContext(ssrc)->msg;
    
    if(msg->bit == msg->size * 8)
        return -1;
    
    int oldBitEnc = msg->bit;

    //future idea: loop unroll
    for(int i = 0; i < 384 + has_y2_block * 16; i++) {
        if(qcoeff[i] != 1 && qcoeff[i] != 0 && (!has_y2_block || MOD16(i) != 0 || i > 255)) {
            qcoeff[i] = (qcoeff[i] & 0xFFFE) | getBit(msg->buffer, msg->bit);
            msg->bit++;
            
            if(msg->bit == msg->size * 8)
                break;
        } 
    }

    return msg->bit - oldBitEnc;
    
}

static void flushDecoder(uint32_t ssrc) {

    message_t *msg = getDecoderContext(ssrc)->msg;
    int n_bytes;
    msg->bit = 0; //should be 0

    unsigned char msgType = msg->buffer[2];
    unsigned char sender = msg->buffer[3];
    unsigned char receiver = msg->buffer[4];

    if(msgType == 0x1 && receiver == senderId) {
        uint32_t localSsrc = obtainSsrc(msg);
        context_t *ctx = getEncoderContext(localSsrc);
        ctx->id[ctx->n_ids++] = sender;
    }

    n_bytes = write(decoderFd, msg->buffer, msg->size);

    if(n_bytes == -1)
        error(strerror(errno), "Trying to write to the decoder pipe");

    else if(n_bytes < msg->size)
        error(strerror(errno), "Trying to write to the decoder pipe, wrote less bytes than expected");

}

int readQdctLsb(short *qcoeff, int has_y2_block, uint32_t ssrc) {

    message_t *msg = getDecoderContext(ssrc)->msg;

    //optimization idea: loop unroll
    for(int i = 0; i < 384 + has_y2_block * 16; i++) {
        if(qcoeff[i] != 1 && qcoeff[i] != 0 && (!has_y2_block || MOD16(i) != 0 || i > 255)) {
            setBit(msg->buffer, msg->bit, getLsb(qcoeff[i]));
            msg->bit++;

            if(msg->bit == 16) {
                msg->size = parseHeader(msg->buffer, 0) + 2;
                if (msg->size == 2) { //padding indicating the end of the message in this frame
                    msg->bit = 0;
                    return 1;
                }
            }
            else if(msg->bit == msg->size * 8 && msg->bit > 16) {
                flushDecoder(ssrc);
                return 1;
            }
        }
            
    }
    return 0;

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

void printQdct(short *qcoeff) {

    printf("\nMacroblock:");
    for(int i = 0; i < 400; i++) {
        if (i % 16 == 0)
            printf("\n");
        printf("%d,", qcoeff[i]);
    }
    printf("\n");

}
