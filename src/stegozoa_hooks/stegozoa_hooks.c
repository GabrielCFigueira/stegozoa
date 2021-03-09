#include "stegozoa_hooks.h"
#include <stdio.h>

char msg[] = "AAAAAAAAAAAAAAAAAAAA";

void writeQdctLsb(short *qcoeff) {

    int n_bits = sizeof(msg) * 8;

    //future idea: loop unroll
    for(int i = 0; i < 400 && i < n_bits; i++) {
        short bit = (msg[i / 8] >> (i % 8)) & 1;
        qcoeff[i] = qcoeff[i] & (0xFFFE | bit);
    }
    
}

void printQdct(short *qcoeff) {

    FILE *fp;

    fp = fopen("/home/vagrant/qcoeff.txt", "a");

    if(!fp) {
        printf("Stegozoa: Couldnt open file");
        return;
    }
    for(int i = 0; i < 400; i++) {
        if (i % 16 == 0)
            fprintf(fp, "\n");
        fprintf(fp, "%d", qcoeff[i]);
    }

    fp.close();

}

