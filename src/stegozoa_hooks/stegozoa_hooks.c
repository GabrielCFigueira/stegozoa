#include "stegozoa_hooks.h"
#include "vp8/common/blockd.h"
#include <stdio.h>

char msg[] = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

void writeQdctLsb(MACROBLOCKD *x) {
    
    for(int i = 0; i < 400; i++) {
        x->qcoeff[i] = msg[0];
    }
    
}

void printQdct(MACROBLOCKD *x) {

    for(int i = 0; i < 400; i++) {
        if (i % 16 == 0)
            printf("\n");
        printf("%d", x->qcoeff[i]);
    }

}

