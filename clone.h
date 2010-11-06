#include "Koi.h"


#ifndef CLONE_H
#define CLONE_H

void * find_clone_job(void *pArg);



typedef struct
{
    int row;
    int col;
    int h_metric;
    int s_metric;
    int l_metric;
    int contrast_metric;
    int crc;

    int metric;
}clone_block_metric;

typedef struct
{
    int h;
    int s;
    int l;
}HSL;

HSL RGBtoHSL( guchar r, guchar g, guchar b);

int clone_metric_comp(const void *a, const void *b);

#endif // CLONE_H
