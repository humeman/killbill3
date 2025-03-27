/**
 * A simple binary heap implementation, used as a priority queue for Dijkstra's.
 * 
 * Author: csenneff
 */

#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>

typedef struct {
    void *item;
    uint32_t priority; // non-negative priorities (we're using Dijkstra's)
} binary_heap_node_t;

typedef struct {
    int count;
    int capacity;
    int item_size;
    binary_heap_node_t* items;
} binary_heap_t;

int heap_init(binary_heap_t **heap, int item_size);
void heap_destroy(binary_heap_t *heap);
int heap_insert(binary_heap_t *heap, void *item, uint32_t priority);
int heap_top(binary_heap_t *heap, void *item, uint32_t *priority);
int heap_at(binary_heap_t *heap, int i, void *item, uint32_t *priority);
int heap_remove(binary_heap_t *heap, void *item, uint32_t *priority);
int heap_decrease_priority(binary_heap_t *heap, int (void*, void*), void *target, uint32_t priority);
int heap_size(binary_heap_t *heap);

#endif