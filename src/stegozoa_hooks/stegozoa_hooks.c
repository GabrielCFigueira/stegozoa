#include "stegozoa_hooks.h"
#include "vp8/common/blockd.h"
#include <stdio.h>

char msg[] = "AAAAAAAAAAAAAAAAAAAA";

void writeQdctLsb(MACROBLOCKD *x) {

    int n_bits = sizeof(msg) * 8;

    //future idea: loop unroll
    for(int i = 0; i < 400, i < n_bits; i++) {
        short bit = (msg[i / 8] >> (i % 8)) & 1;
        x->qcoeff[i] = x->qcoeff[i] & 0xFFFE | bit;
    }
    
}

void printQdct(MACROBLOCKD *x) {

    FILE *fp;

    fp = fopen("/home/vagrant/qcoeff.txt", "a");

    for(int i = 0; i < 400; i++) {
        if (i % 16 == 0)
            fprintf(fp, "\n");
        fprintf(fp, "%d", x->qcoeff[i]);
    }

}

