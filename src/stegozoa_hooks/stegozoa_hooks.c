#include "stegozoa_hooks.h"
#include <stdio.h>

char msg[] = "AAAAAAAAAAAAAAAAAAAA";
static int stop = 0;

int writeQdctLsb(short *qcoeff) {

    int embdata = 0;
    //int n_bits = sizeof(msg) * 8;
    //future idea: loop unroll
    for(int i = 0; i < 384 ; i++) {
        short bit = (msg[0] >> (i % 8)) & 1;
        if(i % 16 == 0 && qcoeff[i]) {
            qcoeff[i] = (qcoeff[i] & 0xFFFE) | bit;
            embdata += 1;
        }
            
    }

    return embdata;
    
}

void printQdct(short *qcoeff) {

/*    FILE *fp;

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

    fclose(fp);*/

    if(stop < 920) {
        stop++;
    for(int i = 0; i < 400; i++) {
        if (i % 16 == 0)
            printf("\n");
        printf("%d,", qcoeff[i]);
    }
    }
}

