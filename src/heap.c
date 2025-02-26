#include "heap.h"
#include "macros.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define INITIAL_CAPACITY 10

int heap_init(binary_heap **heap, int item_size) {
    *heap = malloc(sizeof (**heap));
    if (!*heap) RETURN_ERROR("could not allocate heap");
    (*heap)->capacity = INITIAL_CAPACITY;
    (*heap)->items = malloc(sizeof (binary_heap_node) * INITIAL_CAPACITY);
    (*heap)->count = 0;
    (*heap)->item_size = item_size;
    if (!(*heap)->items) RETURN_ERROR("could not allocate heap items");
    return 0;
}

void heap_destroy(binary_heap *heap) {
    int i;
    for (i = 0; i < heap->count; i++) {
        free(heap->items[i].item);
    }
    free(heap->items);
    free(heap);
}

int heap_insert(binary_heap *heap, void *item, uint32_t priority) {
    int i, parent;
    binary_heap_node temp;
    binary_heap_node *new;
    if (heap->count == heap->capacity) {
        heap->capacity *= 2;
        new = realloc(heap->items, sizeof (binary_heap_node) * heap->capacity);
        if (!new) RETURN_ERROR("could not reallocate heap items");
        heap->items = new;
    }
    heap->count++;
    i = heap->count - 1;
    heap->items[i].item = malloc(heap->item_size);
    heap->items[i].priority = priority;
    memcpy(heap->items[i].item, item, heap->item_size);
    while (i > 0) {
        parent = (i - 1) / 2;
        if (heap->items[parent].priority <= heap->items[i].priority) break;
        temp = heap->items[i];
        heap->items[i] = heap->items[parent];
        heap->items[parent] = temp;
        i = parent;
    }
    return 0;
}

int heap_top(binary_heap *heap, void** item) {
    if (heap->count == 0) RETURN_ERROR("attempted to read top of heap while empty");
    *item = heap->items[0].item;
    return 0;
}

int heap_remove(binary_heap *heap, void** item) {
    int i, l, r, target;
    binary_heap_node temp;
    if (heap->count == 0) RETURN_ERROR("attempted to remove top of heap while empty");
    *item = heap->items[0].item;
    heap->items[0] = heap->items[heap->count - 1];
    heap->count--;
    i = 0;
    while (1) {
        l = 2 * i;
        r = l + 1;
        target = i;

        if (l < heap->count && heap->items[l].priority < heap->items[target].priority) {
            target = l;
        }
        if (r < heap->count && heap->items[r].priority < heap->items[target].priority) {
            target = r;
        }

        if (target == i) break;
        temp = heap->items[i];
        heap->items[i] = heap->items[target];
        heap->items[target] = temp;
        i = target;
    }

    return 0;
}

int heap_decrease_priority(binary_heap *heap, int compare (void*, void*), void *target, uint32_t priority) {
    int i, parent;
    binary_heap_node temp;
    // Find the item
    for (i = 0; i < heap->count; i++) {
        if(compare(heap->items[i].item, target) == 0) break;
    }
    if (i == heap->count) RETURN_ERROR("item is not in heap");

    // Update it
    heap->items[i].priority = priority;
    while (i > 0) {
        parent = (i - 1) / 2;
        if (heap->items[parent].priority <= heap->items[i].priority) break;
        temp = heap->items[i];
        heap->items[i] = heap->items[parent];
        heap->items[parent] = temp;
        i = parent;
    }
    return 0;
}

int heap_size(binary_heap *heap) {
    return heap->count;
}