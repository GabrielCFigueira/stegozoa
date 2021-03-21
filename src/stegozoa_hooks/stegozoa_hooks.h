#ifndef STEGOZOA_HOOKS_H_
#define STEGOZOA_HOOKS_H_

int writeQdctLsb(short *qcoeff);

void printQdct(short *qcoeff, short *qcoeffBlock);

void readQdctLsb(short *qcoeff);

void writeQdct(short *qcoeff);
void readQdct(short *qcoeff);

#endif //STEGOZOA_HOOKS_H_
