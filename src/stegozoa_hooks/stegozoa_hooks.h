#ifndef STEGOZOA_HOOKS_H_
#define STEGOZOA_HOOKS_H_

#include <stdint.h>

#define BUFFER_LEN 17000
#define MSG_SIZE 16378 //2*16 - 6
#define NPEERS 16

typedef struct message {
	unsigned char buffer[BUFFER_LEN];
	int bit;
	int size;

	struct message *next;
} message_t;

typedef struct context {
	message_t *msg;
	uint32_t ssrc;
	int id[NPEERS];
	int n_ids;
} context_t;


void flushEncoder(uint32_t ssrc, int simulcast);

int writeQdctLsb(short *qcoeff, int has_y2_block, uint32_t ssrc);
int readQdctLsb(short *qcoeff, int has_y2_block, uint32_t ssrc);

int initializeEmbbed();
int initializeExtract();
int isEmbbedInitialized();
int isExtractInitialized();


#endif //STEGOZOA_HOOKS_H_
