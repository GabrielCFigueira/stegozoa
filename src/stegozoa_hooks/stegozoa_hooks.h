#ifndef STEGOZOA_HOOKS_H_
#define STEGOZOA_HOOKS_H_

int writeQdctLsb(short *qcoeff, int has_y2_block);
int readQdctLsb(short *qcoeff, int has_y2_block);

int fetchData(int currentFrame);
int initialize();
int isInitialized();

#endif //STEGOZOA_HOOKS_H_
