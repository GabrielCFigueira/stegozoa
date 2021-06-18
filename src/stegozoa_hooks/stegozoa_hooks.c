#include "stegozoa_hooks.h"
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>


#define MASK 0xFE
#define DIVIDE8(num) (num >> 3)
#define MOD8(num) (num & 0x7)
#define MOD16(num) (num & 0xF)
#define getLsb(num) (num & 0x1)
#define rotate(byte, rotation) ((byte << rotation) | (byte >> (8 - rotation)))

#define getBit(A, index) (getLsb(A[DIVIDE8(index)] >> (MOD8(index))))
#define setBit(A, index, bit) \
    (A[DIVIDE8(index)] = (A[DIVIDE8(index)] & rotate(MASK, MOD8(index))) | (bit << MOD8(index)))

#define ENCODER_PIPE "/tmp/stegozoa_encoder_pipe"
#define DECODER_PIPE "/tmp/stegozoa_decoder_pipe"

static int broadcast = 0;

static context_t *encoders[NPEERS];
static int n_encoders = 0;
static context_t *decoders[NPEERS];
static int n_decoders = 0;

static int senderId;

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
    message_t *previousMsg;
    if(msg == NULL)
        ctx->msg = newMsg;
    else {
        do {
            previousMsg = msg;
            msg = msg->next;

            if(msg == NULL || (msg->msgType != 3 && msg->msgType != 4) || 
               msg->msgType > newMsg->msgType || msg->receiverId < newMsg->receiverId || 
               (msg->syn > newMsg->syn && msg->syn - newMsg->syn < 10000)) {
                
                previousMsg->next = newMsg;
                newMsg->next = msg;
                break;

            } else if(msg->msgType == 4 && newMsg->msgType == 4 && 
                    msg->receiverId == newMsg->receiverId && msg->syn == newMsg->syn) //remove duplicates
                return;


        } while (1);
    }
    ctx->n_msg++;
}

static message_t *copyMessage(message_t *msg) {
    message_t *newMsg = newMessage();
    newMsg->bit = msg->bit;
    newMsg->size = msg->size;
    newMsg->receiverId = msg->receiverId;
    newMsg->syn = msg->syn;
    newMsg->msgType = msg->msgType;
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

static int containsId(context_t *ctx, int id) {

    for(int i = 0; i < ctx->n_ids; i++)
        if(ctx->id[i] == id)
            return 1;
    return 0;
}

static context_t *getEncoderContextById(int id) {

    for(int i = 0; i < n_encoders; i++)
        for(int j = 0; j < encoders[i]->n_ids; j++)
            if(encoders[i]->id[j] == id)
                return encoders[i];

    return NULL;
}

static context_t *getDecoderContext(uint32_t ssrc, uint64_t rtpSession) {

    for(int i = 0; i < n_decoders; i++)
        if(decoders[i]->ssrc == ssrc && decoders[i]->rtpSession == rtpSession)
            return decoders[i];

    context_t *ctx = newContext(ssrc);
    ctx->msg = newMessage();
    ctx->n_msg = 1;
    ctx->rtpSession = rtpSession;
    decoders[n_decoders++] = ctx;
    
    return ctx;
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
    
    insertConstant(ssrc, msg->buffer + 10);
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

            unsigned char *flags = newMsg->buffer + 6;


            int msgType = (flags[0] & 0xe0) >> 5;
            //int frag = (flags[0] & 0x10) >> 4;
            int sender = (flags[1] & 0xf0) >> 4;
            int receiver = (flags[1] & 0xf);

            unsigned int syn = parseSize(newMsg->buffer, 8);

            newMsg->msgType = msgType;
            newMsg->receiverId = receiver;
            newMsg->syn = syn;

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
                
                if(msgType == 0) {
                    senderId = sender;
                    message_t *tempMsg;
                    for(int i = 0; i < n_encoders; ++i) {
                        tempMsg = copyMessage(newMsg);
                        insertSsrc(tempMsg, encoders[i]->ssrc);
                        appendMessage(encoders[i], tempMsg);
                    }
                    releaseMessage(newMsg);
                
                } else if(msgType == 3 || msgType == 4) {
                    if(receiver == 15 || broadcast) { // 15 is the broadcast address
                       
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
                    
                
                } else if(msgType == 1 || receiver == 15 || broadcast) {

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

        usleep(2000);
    }

}

static void discardMessage(context_t *ctx) {
    
    message_t *msg = ctx->msg;
    ctx->msg = msg->next;
    releaseMessage(msg);

    ctx->n_msg--;

}

static int obtainMessage(context_t *ctx, unsigned char *message, int size) {
    
    message_t *msg = ctx->msg;

    int toSend = 0;
    while(msg != NULL) {
        int msgSize = (msg->size << 3) - msg->bit;
        int n;

        if(toSend + msgSize > size)
            n = size - toSend;
        else
            n = msgSize;
        
        for(int i = 0; i < n; i++) { 
            message[toSend] = getBit(msg->buffer, msg->bit);
            msg->bit++;
            toSend++;
        }

        if(n != msgSize)
            break;

        msg = msg->next;
        discardMessage(ctx);

    }

    return toSend;
}

/*
 * int h = 7;
 * int w = 4;
 * int H[] = {81, 95, 107, 121};
 * int H_hat[] = {81, 95, 107, 121};
 * int Ht[] = {15, 6, 4, 7, 13, 3, 15};
 * */

/*
 * int h = 2;
 * int w = 2;
 * int H[] = {3, 2};
 * int H_hat[] = {3, 2};
 * int Ht[] = {2, 3};
 * */

const int h = 7;
const int w = 2;
int H[] = {71, 109};
const int H_hat[] = {71, 109};
const int Ht[] = {3, 2, 3, 1, 0, 1, 3};


static void stc(int coverSize, unsigned char *steganogram, unsigned char *message, unsigned char *cover) {

    int indx = 0;
    int indm = 0;
    int hpow = 1 << h;

    printf("hpow: %d\n", hpow);

    int msgSize = coverSize / w;

    float *wght = (float*) malloc(hpow * sizeof(float));
    wght[0] = 0;
    for (int i = 1; i < hpow; i++)
        wght[i] = INFINITY;
    
    printf("Before stack alloc\n");
    fflush(stdout);
    unsigned char *path = malloc(msgSize * w * hpow * sizeof(unsigned char*));

    //unsigned char (*path)[hpow] = malloc(msgSize * w * hpow * sizeof(unsigned char));

    //unsigned char path[msgSize * w][hpow];

    float w0, w1;
    float *newwght = (float*) malloc(hpow * sizeof(float));
    float *temp;

    printf("STC coverSize: %d, msgSize: %d, allocsize: %d\n", coverSize, msgSize, msgSize * w * hpow);
    fflush(stdout);

    //Forward part of the Viterbi algorithm

    for (int i = 0; i < msgSize; i++) {

        if (i >= msgSize - (h - 1)) {
            for (int j = 0; j < w; j++)
                H[j] = H_hat[j] & ((1 << (msgSize - i)) - 1);
            hpow = hpow >> 1;
        }

        for (int j = 0; j < w; j++) {
            for (int k = 0; k < hpow; k++) {

                w0 = wght[k] + cover[indx];
                w1 = wght[k ^ H[j]] + !cover[indx];
                path[indx * (1 << h) + k] = w1 < w0;
                newwght[k] = w1 < w0 ? w1 : w0;
            }
            
            indx++;
            temp = wght;
            wght = newwght;
            newwght = temp;
        
        }

        for (int j = 0; j < hpow >> 1; j++)
            wght[j] = wght[(j << 1) + message[indm]];
        
        for (int j = hpow >> 1; j < hpow; j++)
            wght[j] = INFINITY;

        indm++;
    }
    
    printf("After first part\n");
    fflush(stdout);

    for(int i = 0; i < msgSize * w * (1 << h); i++) {
        fprintf(stdout, "i: %d\n", i);
        fflush(stdout);
        if(path[i] != 0 && path[i] != 1) {
            fprintf(stdout, "What is going on? i: %d, path[i]: %d\n", i, path[i]);
            fflush(stdout);
        }
    }
    
    //Backward part of the Viterbi algorithm

    //float embeddingCost = wght[0];
    int state = 0;
    indx--;
    indm--;
    for (int i = msgSize - 1; i >= 0; i--) {
        state = (state << 1) + message[indm];
        indm--;

        for (int j = w - 1; j >= 0; j--) {
            steganogram[indx] = path[indx * (1 << h) + state];
            state = state ^ (steganogram[indx] ? H[j] : 0);
            printf("state: %d, H[j]: %d, steganogram: %d\n", state, H[j], steganogram[indx] ? H[j] : 0);
            if(state >= 128)
                printf("Alert! indx: %d, indm: %d, message[indm]: %d\n", indx, indm, message[indm]);
            fflush(stdout);
            indx--;
        }
        
        if (i >= msgSize - (h - 1))
            for (int j = 0; j < w; j++)
                H[j] = H_hat[j] & ((1 << (msgSize - i + 1)) - 1);
    }

    free(wght);
    free(newwght);

    printf("After second part\n");
    fflush(stdout);
    for (int i = msgSize * w; i < coverSize; i++)
        steganogram[i] = cover[i];

    free(path);

}

static void reverseStc(unsigned char *steganogram, unsigned char* message, int coverSize) {

    int line = 0;
    for(int i = 0; i < h; i++)
        line += Ht[i] << (w * i);

    int msgSize = coverSize / w;

    for(int i = 0; i < msgSize; i++) {
        int mask = 1;
        int index = 0;
        int bit = 0;

        for(int j = w * (i + 1) - 1; j > w * (i + 1 - h) - 1; j--) {

            if (j < 0)
                break;

            bit ^= steganogram[j] & ((line & mask) >> index);
            index++;
            mask <<= 1;

        }

        message[i] = bit;
    }

}

int flushEncoder(unsigned char *steganogram, unsigned char *cover, uint32_t ssrc, int simulcast, int size) {

    if(pthread_mutex_lock(&barrier_mutex)) {
        error("Who knows", "Trying to acquire the lock");
        return 0; //should abort
    }

    if(broadcast == 0)
        broadcast = simulcast;
    
    context_t *ctx = getEncoderContext(ssrc);


    if(ctx == NULL)
        ctx = createEncoderContext(ssrc);


    if(size < 40) {//size too small
        if(pthread_mutex_unlock(&barrier_mutex)) {
            error("Who knows", "Trying to release the lock");
            return 0; //should abort
        }
        return 0;    
    }

    int msgSize = size / w;

    unsigned char *message = (unsigned char*) malloc(msgSize * sizeof(unsigned char));
    
    int toSend = obtainMessage(ctx, message, msgSize);


    if(pthread_mutex_unlock(&barrier_mutex)) {
        error("Who knows", "Trying to release the lock");
        return 0; //should abort
    }

    stc(size, steganogram, message, cover);
    free(message);


    return toSend;

}


int writeQdctLsb(int **positions, int *row_bits, unsigned char *steganogram, short *qcoeff, int bits) {

    int position;
    int mb_row = 0;
    int index = 0;

    for(int i = 0; i < bits; i++) {
        position = positions[mb_row][index++];
        qcoeff[position] = (qcoeff[position] & 0xFFFE) | steganogram[i];
        
        if(index == row_bits[mb_row]) {
            mb_row++;
            index = 0;
        }
    }
    
    return bits;
}

static void deliverMessage(uint32_t ssrc, uint64_t rtpSession) {

    message_t *msg = getDecoderContext(ssrc, rtpSession)->msg;
    int n_bytes;
    msg->bit = 0;

    unsigned char *flags = msg->buffer + 6;

    int msgType = (flags[0] & 0xe0) >> 5;
    //int frag = (flags[0] & 0x10) >> 4;
    int sender = (flags[1] & 0xf0) >> 4;
    int receiver = (flags[1] & 0xf);

    if(msgType == 1 && receiver == senderId) {
        uint32_t localSsrc = obtainConstant(msg->buffer + 10);
        context_t *ctx = getEncoderContext(localSsrc);
        if(ctx != NULL && !containsId(ctx, sender))
            ctx->id[ctx->n_ids++] = sender;
    }

    n_bytes = write(decoderFd, msg->buffer + 4, msg->size - 4);

    if(n_bytes == -1)
        error(strerror(errno), "Trying to write to the decoder pipe");

    else if(n_bytes < msg->size - 4)
        error(strerror(errno), "Trying to write to the decoder pipe, wrote less bytes than expected");

}

int readQdctLsb(unsigned char* steganogram, int *index, short *qcoeff, int has_y2_block) {

    //optimization idea: loop unroll
    for(int i = 0; i < 384 + has_y2_block * 16; i++) {
        if(qcoeff[i] != 1 && qcoeff[i] != 0 && (!has_y2_block || MOD16(i) != 0 || i > 255)) {
            
            steganogram[*index] = getLsb(qcoeff[i]);
            (*index)++;

        }
            
    }
    return 0;

}

void flushDecoder(unsigned char *steganogram, uint32_t ssrc, uint64_t rtpSession, int size) {


    unsigned char *message = (unsigned char*) malloc(size * sizeof(unsigned char));

    reverseStc(steganogram, message, size);

    message_t *msg = getDecoderContext(ssrc, rtpSession)->msg;

    for(int i = 0; i < size; i++) {
        setBit(msg->buffer, msg->bit, message[i]);
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
            if (msg->size > MSG_SIZE) {
                error("Message size too big", "When extracting bits from qcoeff");
                msg->bit = 0;
            }
            else if(msg->size == 6) {
                error("Message with 0 size", "When extracting bits from qcoeff");
                msg->bit = 0;
            }
        }
        else if(msg->bit == msg->size << 3 && msg->bit > 48)
            deliverMessage(ssrc, rtpSession);
    }

    free(message);
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
