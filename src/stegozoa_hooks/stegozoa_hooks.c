#include "stegozoa_hooks.h"
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>


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

static int broadcast = 0;

static context_t *encoders[NPEERS];
static int n_encoders = 0;
static context_t *decoders[NPEERS];
static int n_decoders = 0;

static unsigned char senderId;

static int encoderFd;
static int decoderFd;
static int embbedInitialized = 0;
static int extractInitialized = 0;

static pthread_t thread;
static pthread_mutex_t barrier_mutex;

static uint32_t constant = 0xC76E;





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
    if(context == NULL) {
        error("null pointer", "allocating new context_t");
        return context; //should abort
    }
    
    context->ssrc = ssrc;
    context->msg = newMessage();
    context->n_msg = 1;
    return context;
}

static void releaseMessage(message_t *message) {
    free(message);
}

static void appendMessage(context_t *ctx, message_t *newMsg) {
    message_t *msg = ctx->msg;
    if(msg == NULL)
        ctx->msg = newMsg;
    else {
        while(msg->next != NULL) msg = msg->next;
        msg->next = newMsg;
    }
    ctx->n_msg++;
}

static void insertMessage(context_t *ctx, message_t *newMsg) {
    message_t *msg = ctx->msg;
    if(msg == NULL)
        ctx->msg = newMsg;
    else {
        newMsg->next = msg->next;
        msg->next = newMsg;
    }
    ctx->n_msg++;
}

static message_t *copyMessage(message_t *msg) {
    message_t *newMsg = newMessage();
    newMsg->bit = msg->bit;
    newMsg->size = msg->size;
    memcpy(newMsg->buffer, msg->buffer, BUFFER_LEN * sizeof(unsigned char));
    return newMsg;
}

static int parseSize(unsigned char array[], int index) {
    int res = 0;

    res = (res | array[index + 1]) << 8;
    res = res | array[index];

    return res;
}

static context_t *getEncoderContext(uint32_t ssrc) {

    for(int i = 0; i < n_encoders; i++)
        if(encoders[i]->ssrc == ssrc)
            return encoders[i];
    return NULL;
}

static context_t *createEncoderContext(uint32_t ssrc) {
    encoders[n_encoders++] = newContext(ssrc);
    return encoders[n_encoders - 1];
}

static context_t *encoderCtxMostMessages() {

    int n_msg = -1;
    context_t *res = NULL;

    for(int i = 0; i < n_encoders; i++)
        if(encoders[i]->n_msg > n_msg) {
            res = encoders[i];
            n_msg = encoders[i]->n_msg;
        }

    return res;
}

static void cloneMessageQueue(context_t *src, context_t *dst) {

    message_t *msgSrc = src->msg;
    dst->msg = copyMessage(msgSrc);
    message_t *msgDst = dst->msg;
    
    while(msgSrc->next != NULL) {
        msgDst->next = copyMessage(msgSrc->next);
        msgSrc = msgSrc->next;
        msgDst = msgDst->next;
    }
    dst->n_msg = src->n_msg;

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


static void insertConstant(uint32_t constant, unsigned char buffer[]) {
    buffer[0] = constant & 0xff;
    buffer[1] = (constant >> 8) & 0xff;
    buffer[2] = (constant >> 16) & 0xff;
    buffer[3] = (constant >> 24) & 0xff;

}

static uint32_t obtainConstant(unsigned char buffer[]) {
    uint32_t constant = 0;

    //probably unnecessary, but must make sure I can shift 24 bits correctly
    uint32_t constant1 = (uint32_t) buffer[0];
    uint32_t constant2 = (uint32_t) buffer[1];
    uint32_t constant3 = (uint32_t) buffer[2];
    uint32_t constant4 = (uint32_t) buffer[3];
    
    constant += constant1;
    constant += constant2 << 8;
    constant += constant3 << 16;
    constant += constant4 << 24;

    return constant;

}

static void shiftConstant(unsigned char buffer[]) { //the constant is in a 4 byte array
    
    buffer[0] = (buffer[0] >> 1) | (buffer[1] << 7);
    buffer[1] = (buffer[1] >> 1) | (buffer[2] << 7);
    buffer[2] = (buffer[2] >> 1) | (buffer[3] << 7);
    buffer[3] = buffer[3] >> 1;


}

static void insertSsrc(message_t *msg, uint32_t ssrc) {
    msg->size += 4;

    msg->buffer[4] = (msg->size - 6) & 0xff; //msg->size - 6 because of the initial constant and size header
    msg->buffer[5] = ((msg->size - 6) >> 8) & 0xff;
    
    insertConstant(ssrc, msg->buffer + 11);
}


static void *fetchDataThread(void *args) {
    
    while(1) {
        unsigned char header[2];
        int read_bytes = read(encoderFd, header, 2);

        if(read_bytes == -1) {
            error(strerror(errno), "Trying to read from the encoder pipe");
            read_bytes = 0;
        }

        if (read_bytes > 0) {

            message_t *newMsg = newMessage();

            insertConstant(constant, newMsg->buffer);
            newMsg->buffer[4] = header[0];
            newMsg->buffer[5] = header[1];

            int size = parseSize(header, 0);
            
            read_bytes = read(encoderFd, newMsg->buffer + 6, size);

            unsigned char msgType = newMsg->buffer[6];
            unsigned char sender = newMsg->buffer[7];
            unsigned char receiver = newMsg->buffer[8];

            if(pthread_mutex_lock(&barrier_mutex)) {
                error("Who knows", "Trying to acquire the lock");
                continue; //should abort
            }
            
            if(size > MSG_SIZE) {
                error("Message too big", "Parsing the header of the new message");
                releaseMessage(newMsg);

            } else if(read_bytes != size) {
                error(strerror(errno), "Trying to read from the encoder pipe after reading the header!");
                releaseMessage(newMsg);
            
            } else {
                newMsg->size = read_bytes + 6; //constant + size header (4 + 2)
                printf("Consegui ler %d bytes\n", read_bytes);
                fflush(stdout);
                
                if(msgType == 0x0) {
                    senderId = sender;
                    message_t *tempMsg;
                    for(int i = 0; i < n_encoders; ++i) {
                        tempMsg = copyMessage(newMsg);
                        insertSsrc(tempMsg, encoders[i]->ssrc);
                        appendMessage(encoders[i], tempMsg);
                    }
                    releaseMessage(newMsg);
                
                } else if(msgType == 0x3 || msgType == 0x4) {
                    if(receiver == 0xff || broadcast) {
                       
                        for(int i = 0; i < n_encoders; ++i) {
                            insertMessage(encoders[i], newMsg);
                            newMsg = copyMessage(newMsg);
                        }
                        releaseMessage(newMsg);

                    } else {
                        context_t *ctxById = getEncoderContextById(receiver);
                        if(ctxById == NULL)
                            error("No context exists for this id", "Sending new message");
                        else
                            insertMessage(ctxById, newMsg);
                    }
                    
                
                } else if(msgType == 0x1 || receiver == 0xff || broadcast) {

                    for(int i = 0; i < n_encoders; ++i) {
                        appendMessage(encoders[i], newMsg);
                        newMsg = copyMessage(newMsg);
                    }
                    releaseMessage(newMsg);

                } else {
                    context_t *ctxById = getEncoderContextById(receiver);
                    if(ctxById == NULL)
                        error("No context exists for this id", "Sending new message");
                    else
                        appendMessage(ctxById, newMsg);
                }

            }

            if(pthread_mutex_unlock(&barrier_mutex)) {
                error("Who knows", "Trying to release the lock");
                continue; //should abort
            }
        }
    }

}

static void discardMessage(context_t *ctx) {
    
    message_t *msg = ctx->msg;
    ctx->msg = msg->next;
    releaseMessage(msg);

    ctx->n_msg--;

    if(ctx->msg == NULL) {
        msg = newMessage();
        insertConstant(constant, msg->buffer);
        msg->buffer[5] = '\0';
        msg->buffer[6] = '\0';
        msg->size = 6;
        appendMessage(ctx, msg);
    }

}

void flushEncoder(uint32_t ssrc, int simulcast) {

    if(pthread_mutex_lock(&barrier_mutex)) {
        error("Who knows", "Trying to acquire the lock");
        return; //should abort
    }

    if(broadcast == 0)
        broadcast = simulcast;
    
    context_t *ctx = getEncoderContext(ssrc);

    if(ctx == NULL) {
        ctx = createEncoderContext(ssrc);
        if(broadcast) {
            context_t *best = encoderCtxMostMessages();
            fprintf(stdout, "Time to Clone! %d\n", best->n_msg);
            fflush(stdout);
            if(best != ctx)
                cloneMessageQueue(best, ctx);
        }
    }

    message_t *msg = ctx->msg;
    
    if(msg->bit == msg->size * 8) //discard current message
        discardMessage(ctx);
    
            
    if(pthread_mutex_unlock(&barrier_mutex)) {
        error("Who knows", "Trying to release the lock");
        return; //should abort
    }

}


int writeQdctLsb(short *qcoeff, int has_y2_block, uint32_t ssrc) {

    context_t *ctx = getEncoderContext(ssrc);
    message_t *msg = ctx->msg;
    
    if(msg->bit == msg->size * 8 && msg->size == 6)
        return -1;
    
    int rate = 0;

    //future idea: loop unroll
    for(int i = 0; i < 384 + has_y2_block * 16; i++) {
        if(qcoeff[i] != 1 && qcoeff[i] != 0 && (!has_y2_block || MOD16(i) != 0 || i > 255)) {
            
            if(msg->bit == msg->size * 8) {
                if(msg->size == 6)
                    break;
                else {
                    
                    if(pthread_mutex_lock(&barrier_mutex)) {
                        error("Who knows", "Trying to acquire the lock");
                        return -1; //should abort
                    }
                    
                    discardMessage(ctx);
                    
                    if(pthread_mutex_unlock(&barrier_mutex)) {
                        error("Who knows", "Trying to release the lock");
                        return -1; //should abort
                    }

                }
            }

            qcoeff[i] = (qcoeff[i] & 0xFFFE) | getBit(msg->buffer, msg->bit);
            msg->bit++;
            rate++;
            
        } 
    }

    return rate;
    
}

static void flushDecoder(uint32_t ssrc) {

    message_t *msg = getDecoderContext(ssrc)->msg;
    int n_bytes;
    msg->bit = 0; //should be 0

    unsigned char msgType = msg->buffer[6];
    unsigned char sender = msg->buffer[7];
    unsigned char receiver = msg->buffer[8];

    if(msgType == 0x1 && receiver == senderId) {
        uint32_t localSsrc = obtainConstant(msg->buffer + 11);
        context_t *ctx = getEncoderContext(localSsrc);
        ctx->id[ctx->n_ids++] = (int) sender;
    }

    n_bytes = write(decoderFd, msg->buffer + 4, msg->size - 4);

    if(n_bytes == -1)
        error(strerror(errno), "Trying to write to the decoder pipe");

    else if(n_bytes < msg->size - 4)
        error(strerror(errno), "Trying to write to the decoder pipe, wrote less bytes than expected");

}

int readQdctLsb(short *qcoeff, int has_y2_block, uint32_t ssrc) {

    message_t *msg = getDecoderContext(ssrc)->msg;

    //optimization idea: loop unroll
    for(int i = 0; i < 384 + has_y2_block * 16; i++) {
        if(qcoeff[i] != 1 && qcoeff[i] != 0 && (!has_y2_block || MOD16(i) != 0 || i > 255)) {
            setBit(msg->buffer, msg->bit, getLsb(qcoeff[i]));
            msg->bit++;
            
            if(msg->bit == 32) {
                uint32_t newConstant = obtainConstant(msg->buffer);
                if(newConstant != constant) {
                    shiftConstant(msg->buffer);
                    msg->bit--;
                }
            }
            else if(msg->bit == 48) {
                msg->size = parseSize(msg->buffer + 4, 0) + 6;
                if (msg->size == 6 || msg->size > MSG_SIZE) {
                    msg->bit = 0;
                    return 1;
                }
            }
            else if(msg->bit == msg->size * 8 && msg->bit > 48)
                flushDecoder(ssrc);
        }
            
    }
    return 0;

}


int initializeExtract() {

    static int dontRepeat = 0;

    decoderFd = open(DECODER_PIPE, O_WRONLY);
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

    encoderFd = open(ENCODER_PIPE, O_RDONLY);
    if(encoderFd < 1) {
        if(!dontRepeat)
            error(strerror(errno), "Trying to open the encoder pipe for reading");
        dontRepeat = 1;
        return 1;
    }
    
    else if(pthread_create(&thread, NULL, fetchDataThread, NULL)) {
        error("Who knows", "Creating the encoder thread");
        return 1;
    }
    
    if(pthread_mutex_init(&barrier_mutex, NULL) != 0) {
        error("Who knows", "Initializing mutex");
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
