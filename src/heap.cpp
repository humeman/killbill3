#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

#include "heap.h"
#include "macros.h"

#define INITIAL_CAPACITY 10

int heap_init(binary_heap_t **heap, int item_size)
{
    *heap = (binary_heap_t *) malloc(sizeof (**heap));
    if (!*heap)
        RETURN_ERROR("could not allocate heap");
    (*heap)->capacity = INITIAL_CAPACITY;
    (*heap)->items = (binary_heap_node_t *) malloc(sizeof (binary_heap_node_t) * INITIAL_CAPACITY);
    (*heap)->count = 0;
    (*heap)->item_size = item_size;
    if (!(*heap)->items)
        RETURN_ERROR("could not allocate heap items");
    return 0;
}

void heap_destroy(binary_heap_t *heap)
{
    int i;
    for (i = 0; i < heap->count; i++)
    {
        free(heap->items[i].item);
    }
    free(heap->items);
    free(heap);
}

int heap_insert(binary_heap_t *heap, void *item, uint32_t priority)
{
    int i, parent;
    binary_heap_node_t temp;
    binary_heap_node_t *new_node;
    if (heap->count == heap->capacity)
    {
        heap->capacity *= 2;
        new_node = (binary_heap_node_t *) realloc(heap->items, sizeof (binary_heap_node_t) * heap->capacity);
        if (!new_node)
            RETURN_ERROR("could not reallocate heap items");
        heap->items = new_node;
    }
    heap->count++;
    i = heap->count - 1;
    heap->items[i].item = malloc(heap->item_size);
    printf("%d\n", priority);
    heap->items[i].priority = priority;
    memcpy(heap->items[i].item, item, heap->item_size);
    while (i > 0)
    {
        parent = (i - 1) / 2;
        if (heap->items[parent].priority <= heap->items[i].priority)
            break;
        temp = heap->items[i];
        heap->items[i] = heap->items[parent];
        heap->items[parent] = temp;
        i = parent;
    }
    return 0;
}

int heap_top(binary_heap_t *heap, void *item, uint32_t *priority)
{
    if (heap->count == 0)
        RETURN_ERROR("attempted to read top of heap while empty");
    memcpy(item, heap->items[0].item, heap->item_size);
    *priority = heap->items[0].priority;
    return 0;
}

int heap_at(binary_heap_t *heap, int i, void *item, uint32_t *priority)
{
    if (i >= heap->count || i < 0)
        RETURN_ERROR("attempted to read heap item at invalid index");
    memcpy(item, heap->items[i].item, heap->item_size);
    *priority = heap->items[i].priority;
    return 0;
}

int heap_remove(binary_heap_t *heap, void *item, uint32_t *priority)
{
    int i, l, r, target;
    binary_heap_node_t temp;
    if (heap->count == 0)
        RETURN_ERROR("attempted to remove top of heap while empty");
    memcpy(item, heap->items[0].item, heap->item_size);
    *priority = heap->items[0].priority;
    free(heap->items[0].item);
    heap->items[0] = heap->items[heap->count - 1];
    heap->count--;
    i = 0;
    while (1)
    {
        l = 2 * i + 1;
        r = l + 1;
        target = i;

        if (l < heap->count && heap->items[l].priority < heap->items[target].priority)
        {
            target = l;
        }
        if (r < heap->count && heap->items[r].priority < heap->items[target].priority)
        {
            target = r;
        }

        if (target == i)
            break;
        temp = heap->items[i];
        heap->items[i] = heap->items[target];
        heap->items[target] = temp;
        i = target;
    }

    return 0;
}

int heap_decrease_priority(binary_heap_t *heap, int compare(void *, void *), void *target, uint32_t priority)
{
    int i, parent;
    binary_heap_node_t temp;
    // Find the item
    for (i = 0; i < heap->count; i++)
    {
        if (compare(heap->items[i].item, target) == 0)
            break;
    }
    if (i == heap->count)
        RETURN_ERROR("item is not in heap");

    // Update it
    heap->items[i].priority = priority;
    while (i > 0)
    {
        parent = (i - 1) / 2;
        if (heap->items[parent].priority <= heap->items[i].priority)
            break;
        temp = heap->items[i];
        heap->items[i] = heap->items[parent];
        heap->items[parent] = temp;
        i = parent;
    }
    return 0;
}

int heap_size(binary_heap_t *heap)
{
    return heap->count;
}