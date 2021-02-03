#ifndef HURT_BITOW_LIST_H
#define HURT_BITOW_LIST_H

#include <stdio.h>
#include <stdlib.h>

typedef struct list {
    int value;
    struct list *ptr;
} list;

list *create();

int get( list * );

list *del( list * );

void put( list *, int );

#endif //HURT_BITOW_LIST_H
