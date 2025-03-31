/**
 * A simple binary heap implementation, used as a priority queue for Dijkstra's.
 * 
 * Author: csenneff
 */

#ifndef HEAP_H
#define HEAP_H

#include <cstdint>

typedef struct {
    void *item;
    uint32_t priority; // non-negative priorities (we're using Dijkstra's)
} binary_heap_node_t;

class binary_heap_t {
    private:
        int count;
        int capacity;
        int item_size;
        binary_heap_node_t* items;
        bool (*compare)(void *, void *);

    public:
        /**
         * Initializes a new binary heap.
         * 
         * Params:
         * - item_size: Size of the data being stored
         * - compare: Comparator (return true if equal) to find items in the heap
         */
        binary_heap_t(int item_size, bool compare(void *, void *));
        ~binary_heap_t();

        /**
         * Inserts an item into the heap.
         * 
         * Params:
         * - item: Pointer to the data to be *copied* into the heap.
         * - priority: Priority to insert with.
         */
        void insert(void *item, uint32_t priority);

        /**
         * Gets (without removing) the top item on the heap.
         * 
         * Params:
         * - item: Pointer to memory where the data will be *copied*.
         * Returns: The priority of the item.
         */
        uint32_t top(void *item);

        /**
         * Gets (without removing) any item on the heap.
         * 
         * Params:
         * - i: Index of the item to get.
         * - item: Pointer to memory where the data will be *copied*.
         * Returns: The priority of the item.
         */
        uint32_t at(int i, void *item);

        /**
         * Removes the top item on the heap.
         * 
         * Params:
         * - item: Pointer to memory where the data will be *copied*.
         * Returns: The priority of the item.
         */
        uint32_t remove(void *item);

        /**
         * Decreases the priority of an item in the heap.
         * 
         * Params:
         * - target: The data to decrease the priority of.
         * - priority: The new priority.
         */
        void decrease_priority(void *target, uint32_t priority);

        /**
         * Gets the size of the heap.
         * 
         * Returns: Number of items.
         */
        int size();
};

#endif