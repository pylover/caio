#include <stdlib.h>
#include <string.h>


int
GARRNAME(array_init)(struct GARRNAME(array) *self, size_t size) {
    self->array = calloc(size, sizeof(GARR_TYPE*));
    memset(self->array, 0, size * sizeof(GARR_TYPE*));
    self->count = 0;
    self->size = size;
    return 0;
}


int
GARRNAME(array_append)(struct GARRNAME(array) *self, GARR_TYPE *item) {
    int i;

    for (i = 0; i < self->size; i++) {
        if (self->array[i] == NULL) {
            goto found;
        }
    }

notfound:
    return -1;

found:
    self->array[i] = item;
    self->count++;
    return i;
}


int
GARRNAME(array_set)(struct GARRNAME(array) *self, GARR_TYPE *item,
        unsigned int index) {
    if (self->size <= index) {
        return -1;
    }

    if (item == NULL) {
        if (self->array[index] != NULL) {
            self->count--;
        }
    }
    else {
        if (self->array[index] == NULL) {
            self->count++;
        }
    }

    self->array[index] = item;
    return 0;
}


GARR_TYPE*
GARRNAME(array_get)(struct GARRNAME(array) *self, unsigned int index) {
    return self->array[index];
}
