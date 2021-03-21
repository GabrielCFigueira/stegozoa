#ifndef STEGOZOA_HOOKS_H_
#define STEGOZOA_HOOKS_H_

int writeQdctLsb(short *qcoeff);

void printQdct(short *qcoeff, short *qcoeffBlock);

void readQdctLsb(short *qcoeff);

void writeQdct(short *qcoeff, char *eobs, int has_y2_block);
void readQdct(short *qcoeff, int has_y2_block);

#endif //STEGOZOA_HOOKS_H_
