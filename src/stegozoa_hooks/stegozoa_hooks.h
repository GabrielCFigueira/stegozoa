#ifndef STEGOZOA_HOOKS_H_
#define STEGOZOA_HOOKS_H_

int writeQdctLsb(short *qcoeff, int has_y2_block);

void printQdct(short *qcoeff);

void readQdctLsb(short *qcoeff, int has_y2_block);

void writeQdct(short *qcoeff, char *eobs, int has_y2_block);
void readQdct(short *qcoeff, int has_y2_block);

#endif //STEGOZOA_HOOKS_H_
