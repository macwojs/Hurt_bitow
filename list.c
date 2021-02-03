#include "list.h"

list *create() {
    list *new = ( list * ) malloc( sizeof( list ));
    new->value = -1;
    new->ptr = NULL;
    return new;
}

int get( list *a ) {
    return a->value;
}

list *del( list *a ) {
    if ( a->ptr == NULL) {
        free( a );
        return create();
    }
    else {
        list *w = a;
        a = a->ptr;
        free( w );
        return a;
    }

}

void put( list *a, int value ) {
    if ( a->value == -1 ) {
        a->value = value;
    }
    else {
        while ( a->ptr != NULL) {
            a = a->ptr;
        }
        list *new = ( list * ) malloc( sizeof( list ));
        new->value = value;
        new->ptr = NULL;
        a->ptr = new;
    }
}
