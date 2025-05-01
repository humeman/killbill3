/**
 * A simple binary heap implementation, used as a priority queue for Dijkstra's.
 *
 * Author: csenneff
 */

#ifndef HEAP_H
#define HEAP_H

#include <cstdint>
#include <cstdlib>
#include <vector>

#include "macros.h"

template <class T>
class BinaryHeap {
    private:
        class binary_heap_node_t {
            public:
                T item;
                uint32_t priority; // non-negative priorities (we're using Dijkstra's)
        };

        std::vector<binary_heap_node_t> items;

    public:
        /**
         * Initializes a new binary heap.
         */
        BinaryHeap() {}
        ~BinaryHeap() {}

        /**
         * Inserts an item into the heap.
         *
         * Params:
         * - item: Pointer to the data to be *copied* into the heap.
         * - priority: Priority to insert with.
         */
        void insert(T item, uint32_t priority) {
            int i, parent;
            binary_heap_node_t new_node;
            binary_heap_node_t temp;

            if (items.size() == INT32_MAX) {
                // Something has gone horribly wrong either way.
                // But we use ints for the size, so we have to error here to not wrap around.
                throw dungeon_exception(__PRETTY_FUNCTION__, "heap is full");
            }

            new_node.item = item;
            new_node.priority = priority;
            items.push_back(new_node);
            i = items.size() - 1;
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

        /**
         * Gets (without removing) the top item on the heap.
         *
         * Returns: The top item.
         */
        T top() {
            if (items.size() == 0)
                throw dungeon_exception(__PRETTY_FUNCTION__, "attempted to read top of heap while empty");
            return items[0].item;
        }

        /**
          * Gets the priority of the item on the top of the heap.
          *
          * Returns: The priority of the item.
          */
        uint32_t top_priority() {
            return priority_at(0);
        }

        /**
         * Gets (without removing) any item on the heap.
         *
         * Params:
         * - i: Index of the item to get.
         * Returns: The item.
         */
        T at(int i) {
            if (i >= ((int) items.size()) || i < 0)
                throw dungeon_exception(__PRETTY_FUNCTION__, "attempted to read heap item at invalid index");
            return items[i].item;
        }

        /**
          * Gets the priority of any item on the heap.
          *
          *
          * Params:
          * - i: Index of the item to get.
          * Returns: The priority of the item.
          */
        uint32_t priority_at(int i) {
            if (i >= ((int) items.size()) || i < 0)
                throw dungeon_exception(__PRETTY_FUNCTION__, "attempted to read heap item at invalid index");
            return items[i].priority;
        }

        /**
         * Removes the top item on the heap.
         *
         * Returns: The item.
         */
        T remove() {
            int i, l, r, target;
            T removed;
            binary_heap_node_t temp;
            if (items.size() == 0)
                throw dungeon_exception(__PRETTY_FUNCTION__, "attempted to remove top of heap while empty");
            removed = items[0].item;
            items[0] = items[items.size() - 1];
            items.erase(items.end());
            i = 0;
            while (1)
            {
                l = 2 * i + 1;
                r = l + 1;
                target = i;

                if (l < ((int) items.size()) && items[l].priority < items[target].priority)
                {
                    target = l;
                }
                if (r < ((int) items.size()) && items[r].priority < items[target].priority)
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
            return removed;
        }

        /**
         * Decreases the priority of an item in the heap.
         *
         * Params:
         * - target: The data to decrease the priority of.
         * - priority: The new priority.
         */
        void decrease_priority(T target, uint32_t priority) {
            int i, parent;
            binary_heap_node_t temp;
            // Find the item
            for (i = 0; i < (int) items.size(); i++)
            {
                if (items[i].item == target)
                    break;
            }
            if (i == (int) items.size())
                throw dungeon_exception(__PRETTY_FUNCTION__, "item is not in heap");

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

        /**
         * Gets the size of the heap.
         *
         * Returns: Number of items.
         */
        int size() {
            return (int) items.size();
        }
};

#endif
