#ifndef STEGOZOA_HOOKS_H_
#define STEGOZOA_HOOKS_H_

int writeQdctLsb(short *qcoeff, int has_y2_block, int currentFrame);
int readQdctLsb(short *qcoeff, int has_y2_block);

int initializeEmbbed();
int initializeExtract();
int isEmbbedInitialized();
int isExtractInitialized();


#endif //STEGOZOA_HOOKS_H_
