#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

#include "heap.h"
#include "macros.h"
#include <stdexcept>

#define INITIAL_CAPACITY 10

binary_heap_t::binary_heap_t(int item_size, bool compare(void *, void *))
{
    capacity = INITIAL_CAPACITY;
    items = (binary_heap_node_t *) malloc(sizeof (binary_heap_node_t) * INITIAL_CAPACITY);
    if (!items)
        throw std::runtime_error("could not allocate heap items");
    count = 0;
    this->item_size = item_size;
    this->compare = compare;
}

binary_heap_t::~binary_heap_t()
{
    int i;
    for (i = 0; i < count; i++)
    {
        free(items[i].item);
    }
    free(items);
}

void binary_heap_t::insert(void *item, uint32_t priority)
{
    int i, parent;
    binary_heap_node_t temp;
    binary_heap_node_t *new_node;
    if (count == capacity)
    {
        capacity *= 2;
        new_node = (binary_heap_node_t *) realloc(items, sizeof (binary_heap_node_t) * capacity);
        if (!new_node)
            throw std::runtime_error("could not reallocate heap items");
        items = new_node;
    }
    i = count;
    items[i].item = malloc(item_size);
    if (!(items[i].item)) throw std::runtime_error("failed to allocate item");
    count++;
    items[i].priority = priority;
    memcpy(items[i].item, item, item_size);
    while (i > 0)
    {
        parent = (i - 1) / 2;
        if (items[parent].priority <= items[i].priority)
            break;
        temp = items[i];
        items[i] = items[parent];
        items[parent] = temp;
        i = parent;
    }
}

void binary_heap_t::top(void *item, uint32_t *priority)
{
    if (count == 0)
        throw std::runtime_error("attempted to read top of heap while empty");
    memcpy(item, items[0].item, item_size);
    *priority = items[0].priority;
}

void binary_heap_t::at(int i, void *item, uint32_t *priority)
{
    if (i >= count || i < 0)
        throw std::runtime_error("attempted to read heap item at invalid index");
    memcpy(item, items[i].item, item_size);
    *priority = items[i].priority;
}

void binary_heap_t::remove(void *item, uint32_t *priority)
{
    int i, l, r, target;
    binary_heap_node_t temp;
    if (count == 0)
        throw std::runtime_error("attempted to remove top of heap while empty");
    memcpy(item, items[0].item, item_size);
    *priority = items[0].priority;
    free(items[0].item);
    items[0] = items[count - 1];
    count--;
    i = 0;
    while (1)
    {
        l = 2 * i + 1;
        r = l + 1;
        target = i;

        if (l < count && items[l].priority < items[target].priority)
        {
            target = l;
        }
        if (r < count && items[r].priority < items[target].priority)
        {
            target = r;
        }

        if (target == i)
            break;
        temp = items[i];
        items[i] = items[target];
        items[target] = temp;
        i = target;
    }
}

void binary_heap_t::decrease_priority(void *target, uint32_t priority)
{
    int i, parent;
    binary_heap_node_t temp;
    // Find the item
    for (i = 0; i < count; i++)
    {
        if (compare(items[i].item, target) == 0)
            break;
    }
    if (i == count)
        throw std::runtime_error("item is not in heap");

    // Update it
    items[i].priority = priority;
    while (i > 0)
    {
        parent = (i - 1) / 2;
        if (items[parent].priority <= items[i].priority)
            break;
        temp = items[i];
        items[i] = items[parent];
        items[parent] = temp;
        i = parent;
    }
}

int binary_heap_t::size()
{
    return count;
}