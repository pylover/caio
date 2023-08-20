#include <stdlib.h>
#include <string.h>


int
GSTACKNAME(init)(struct GSTACKSELF() *self, size_t size) {
    self->stack = calloc(size, sizeof(GSTACK_TYPE*));
    if (self->stack == NULL) {
        return -1;
    }
    memset(self->stack, 0, size * sizeof(GSTACK_TYPE*));
    self->count = 0;
    self->size = size;
    return 0;
}


void
GSTACKNAME(deinit)(struct GSTACKSELF() *self) {
    if (self->stack == NULL) {
        return;
    }
    free(self->stack);
}


int
GSTACKNAME(push)(struct GSTACKSELF() *self, GSTACK_TYPE *item) {
    int index;

    if (item == NULL) {
        return -1;
    }

    if (GSTACK_ISFULL(self)) {
        return -1;
    }

    index = self->count++;
    self->stack[index] = item;
    return index;
}


GSTACK_TYPE*
GSTACKNAME(pop)(struct GSTACKSELF() *self) {
    if (self->count <= 0) {
        return NULL;
    }

    return self->stack[--self->count];
}


GSTACK_TYPE*
GSTACKNAME(last)(struct GSTACKSELF() *self) {
    if (self->count <= 0) {
        return NULL;
    }

    return self->stack[self->count - 1];
}
