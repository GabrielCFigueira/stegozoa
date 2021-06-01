#ifndef STEGOZOA_HOOKS_H_
#define STEGOZOA_HOOKS_H_

#include <stdint.h>

#define BUFFER_LEN 1038
#define MSG_SIZE 1038 //2*10 + header (14 bytes)
#define NPEERS 256

typedef struct message {
	unsigned char buffer[BUFFER_LEN];
	int bit;
	int size;

	struct message *next;
} message_t;

typedef struct context {
	message_t *msg;
	uint32_t ssrc;
	uint64_t rtpSession;
	int n_msg;
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
