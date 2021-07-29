#ifndef STEGOZOA_HOOKS_H_
#define STEGOZOA_HOOKS_H_

#include <stdint.h>
#include "macros.h"

#define BUFFER_LEN 270
#define MSG_SIZE 270 //2*8 + header (14 bytes)
#define NPEERS 256

#define MAX_CAPACITY 4000
#define HEIGHT 7
#define HPOW 128 //2 ** h
#define WIDTH 4

typedef struct message {
	unsigned char buffer[BUFFER_LEN];
	int bit;
	int size;

	int msgType;
	int receiverId;
	unsigned int syn;

	struct message *next;
} message_t;

typedef struct stcdata {
	unsigned char path[HPOW * MAX_CAPACITY];
	unsigned char messagePath[HPOW * MAX_CAPACITY / WIDTH];
	float wght[HPOW];
	float newwght[HPOW];
	unsigned char message[MAX_CAPACITY / WIDTH];
	unsigned char cover[MAX_CAPACITY];
	unsigned char steganogram[MAX_CAPACITY];
} stc_data_t;

typedef struct context {
	message_t *msg;
	uint32_t ssrc;
	uint64_t rtpSession;
	int n_msg;
	int id[NPEERS];
	int n_ids;
	stc_data_t *stcData;
} context_t;



stc_data_t *getStcData(uint32_t ssrc);
int flushEncoder(uint32_t ssrc, int simulcast, int size);
void flushDecoder(unsigned char *steganogram, uint32_t ssrc, uint64_t rtpSession, int size);

int writeQdctLsb(int **positions, int *row_bits, int n_rows, unsigned char* steganogram, short *qcoeff, int bits);
void readQdctLsb(int **positions, int *row_bits, int n_rows, unsigned char* steganogram, short *qcoeff, int bits);

int initializeEmbbed();
int initializeExtract();
int isEmbbedInitialized();
int isExtractInitialized();

#endif //STEGOZOA_HOOKS_H_
