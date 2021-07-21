#ifndef STEGOZOA_HOOKS_H_
#define STEGOZOA_HOOKS_H_

#include <stdint.h>
#include "macros.h"

#define BUFFER_LEN 270
#define MSG_SIZE 270 //2*8 + header (14 bytes)
#define NPEERS 256

const int maxCapacity = 4000;

const int h = 7;
const int hpow = (1 << h);
const int w = 4;
const int H_hat[] = {81, 95, 107, 121};
const int Ht[] = {15, 6, 4, 7, 13, 3, 15};

/*
 * const int h = 7;
 * const int w = 3;
 * const int H_hat[] = {95, 101, 121};
 * const int Ht[] = {7, 4, 6, 5, 5, 3, 7};
 * */

/*
 * const int h = 7;
 * const int w = 2;
 * const int H_hat[] = {71, 109};
 * const int Ht[] = {3, 2, 3, 1, 0, 1, 3};
 * */

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
	unsigned char path[hpow * maxCapacity];
	unsigned char messagePath[hpow * maxCapacity / w];
	float wght[hpow];
	float newwght[hpow];
	unsigned char message[maxCapacity / w];
	unsigned char cover[maxCapacity];
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
