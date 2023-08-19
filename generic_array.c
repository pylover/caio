#include <stdlib.h>
#include <string.h>


int
GARRNAME(init)(struct GARRSELF() *self, size_t size) {
    self->array = calloc(size, sizeof(GARR_TYPE*));
    if (self->array == NULL) {
        return -1;
    }
    memset(self->array, 0, size * sizeof(GARR_TYPE*));
    self->count = 0;
    self->size = size;
    return 0;
}


void
GARRNAME(deinit)(struct GARRSELF() *self) {
    if (self->array == NULL) {
        return;
    }
    free(self->array);
}


int
GARRNAME(append)(struct GARRSELF() *self, GARR_TYPE *item) {
    int i;

    if (item == NULL) {
        return -1;
    }

    if (GARR_ISFULL(self)) {
        return -1;
    }

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
GARRNAME(set)(struct GARRSELF() *self, GARR_TYPE *item,
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
GARRNAME(get)(struct GARRSELF() *self, unsigned int index) {
    if (self->size <= index) {
        return NULL;
    }

    return self->array[index];
}


int
GARRNAME(del)(struct GARRSELF() *self, unsigned int index) {
    if (self->size <= index) {
        return -1;
    }

    if (self->array[index] == NULL) {
        return -1;
    }

    self->count--;
    self->array[index] = NULL;
    return 0;
}
