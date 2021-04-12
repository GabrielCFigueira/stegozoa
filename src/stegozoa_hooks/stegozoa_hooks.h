#ifndef STEGOZOA_HOOKS_H_
#define STEGOZOA_HOOKS_H_

#include <stdint.h>

#define BUFFER_LEN 10500

typedef struct encoder {
	unsigned char buffer[BUFFER_LEN];
	int bit;
	int size;

	uint32_t ssrc;
	
	struct encoder *next;
} encoder_t;

typedef struct decoder {
	unsigned char buffer[BUFFER_LEN];
	int bit;
	int size;

	uint32_t ssrc;

	struct decoder *next;
} decoder_t;


encoder_t *newEncoder();
decoder_t *newDecoder();

void releaseEncoder(encoder_t *encoder);
void releaseDecoder(decoder_t *decoder);

void fetchData();

int writeQdctLsb(short *qcoeff, int has_y2_block);
int readQdctLsb(short *qcoeff, int has_y2_block);

int initializeEmbbed();
int initializeExtract();
int isEmbbedInitialized();
int isExtractInitialized();


#endif //STEGOZOA_HOOKS_H_
