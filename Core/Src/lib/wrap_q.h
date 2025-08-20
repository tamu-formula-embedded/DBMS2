#ifndef WRAP_QUEUE_H
#define WRAP_QUEUE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief A fixed-size circular queue that stores generic items using a user-provided buffer.
 *
 * The queue operates in FIFO order. It does not support resizing or dynamic memory allocation.
 * Data is stored contiguously in the provided buffer and accessed using void pointers.
 */
typedef struct _wrap_queue
{
    uint8_t* buffer;        /**< Raw pointer to user-provided buffer. */
    size_t item_size;       /**< Size of each item in bytes. */
    size_t capacity;        /**< Maximum number of items that can be stored in the queue. */

    size_t head;            /**< Index of the front (oldest) element in the queue. */
    size_t tail;            /**< Index of the next insertion point. */
    size_t count;           /**< Current number of items in the queue. */
} wrap_queue_t;

/**
 * @brief Initializes the wrap-around queue using the given buffer.
 *
 * @param q           Pointer to the queue structure to initialize.
 * @param buffer      Pointer to the user-provided memory buffer.
 * @param item_count  Number of items that the buffer can hold.
 * @param item_size   Size of each item in bytes.
 *
 * @return 0 on success, or a negative error code on invalid arguments.
 */
int wrap_queue_init(wrap_queue_t* q, void* buffer, size_t item_count, size_t item_size);

/**
 * @brief Inserts a new item into the queue.
 *
 * @details This function returns a pointer to the memory slot where the new item can be written.
 * The caller must write into the returned slot before the next queue operation.
 *
 * @param q Pointer to the queue.
 * @return Pointer to a writable slot if the queue has space, or NULL if the queue is full.
 */
void* wrap_queue_push(wrap_queue_t* q);

/**
 * @brief Returns a pointer to the item at the front of the queue.
 *
 * @param q Pointer to the queue.
 * @return Pointer to the front item, or NULL if the queue is empty.
 */
void* wrap_queue_front(wrap_queue_t* q);

/**
 * @brief Removes and returns the item at the front of the queue.
 *
 * @param q Pointer to the queue.
 * @return Pointer to the item that was removed, or NULL if the queue was empty.
 */
void* wrap_queue_pop(wrap_queue_t* q);

/**
 * @brief Checks if the queue is currently empty.
 *
 * @param q Pointer to the queue.
 * @return true if the queue is empty, false otherwise.
 */
static inline bool wrap_queue_empty(wrap_queue_t* q)
{
    return q->count == 0;
}

/**
 * @brief Checks if the queue is currently full.
 *
 * @param q Pointer to the queue.
 * @return true if the queue is full, false otherwise.
 */
static inline bool wrap_queue_full(wrap_queue_t* q)
{
    return q->count == q->capacity;
}

#ifdef __cplusplus
}
#endif

#endif // WRAP_QUEUE_H
