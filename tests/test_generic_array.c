#include <cutest.h>


#undef GARR_ITEMISEMPTY
#define GARR_ITEMISEMPTY(c) (c == NULL)
#undef GARR_TYPE
#define GARR_TYPE int
#include "generic_array.h"
#include "generic_array.c"


void
test_generic_array() {
    int index;
    struct int_array arr;
    int_array_init(&arr, 4);

    eqint(4, arr.size);
    eqint(0, arr.count);

    int foo = 1;
    int bar = 2;
    int baz = 3;
    int qux = 4;

    index = int_array_append(&arr, &foo);
    eqint(1, arr.count);
    eqint(0, index);

    index = int_array_append(&arr, &bar);
    eqint(2, arr.count);
    eqint(1, index);

    eqint(0, int_array_set(&arr, &baz, 2));
    eqint(3, arr.count);

    eqint(0, int_array_set(&arr, &baz, 2));
    eqint(3, arr.count);

    index = int_array_append(&arr, &qux);
    eqint(4, arr.count);
    eqint(3, index);

    eqint(0, int_array_set(&arr, NULL, 2));
    eqint(3, arr.count);

    eqint(-1, int_array_set(&arr, NULL, 4));
    eqint(3, arr.count);

    eqptr(&foo, int_array_get(&arr, 0));
    eqptr(&bar, int_array_get(&arr, 1));
    eqptr(NULL, int_array_get(&arr, 2));
    eqptr(&qux, int_array_get(&arr, 3));

    int_array_deinit(&arr);
}


void
main() {
    test_generic_array();
}
