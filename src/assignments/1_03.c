#include "../heap.h"
#include "../macros.h"
#include <stdio.h>

int compare_ints(void *a, void *b) {
    return *((int*) a) - *((int*) b);
}

int main(int argc, char* argv[]) {
    binary_heap* heap;
    if (heap_init(&heap, sizeof (int))) RETURN_ERROR("could not initialize heap");
    for (int i = 10; i > 0; i--) {
        heap_insert(heap, (void *) &i, 10 - i);
    }
    printf("size: %d\n", heap_size(heap));
    int* item;
    heap_top(heap, (void **) &item);
    int target = 1;
    if (heap_decrease_priority(heap, compare_ints, &target, -1)) RETURN_ERROR("couldn't decrease priority");
    printf("top: %d\n", *item);
    for (int i = 0; i < 10; i++) {
        if (heap_remove(heap, (void**) &item)) RETURN_ERROR("couldn't remove from heap");
        printf("removal %i: %d\n", i, *item);
    }
    printf("size: %d\n", heap_size(heap));
    heap_destroy(heap);
    return 0;
}