#ifndef HEAP_H
#define HEAP_H

typedef struct {
    void *item;
    int priority;
} binary_heap_node;

typedef struct {
    int count;
    int capacity;
    int item_size;
    binary_heap_node* items;
} binary_heap;

int heap_init(binary_heap **heap, int item_size);
void heap_destroy(binary_heap *heap);
int heap_insert(binary_heap *heap, void *item, int priority);
int heap_top(binary_heap *heap, void **item);
int heap_remove(binary_heap *heap, void** item);
int heap_decrease_priority(binary_heap *heap, int (void*, void*), void *target, int priority);
int heap_size(binary_heap *heap);

#endif