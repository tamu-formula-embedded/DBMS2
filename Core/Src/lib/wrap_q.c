#include "wrap_q.h"

int wrap_queue_init(wrap_queue_t* q, void* buffer, size_t item_count, size_t item_size)
{
    if (!q || !buffer || item_size == 0 || item_count == 0)
        return -1;

    q->buffer = (uint8_t*)buffer;
    q->item_size = item_size;
    q->capacity = item_count;

    q->head = 0;
    q->tail = 0;
    q->count = 0;

    return 0;
}

void* wrap_queue_push(wrap_queue_t* q)
{
    if (wrap_queue_full(q))
        return NULL;

    void* slot = q->buffer + (q->tail * q->item_size);
    q->tail = (q->tail + 1) % q->capacity;
    q->count++;

    return slot;
}

void* wrap_queue_front(wrap_queue_t* q)
{
    if (wrap_queue_empty(q))
        return NULL;

    return q->buffer + (q->head * q->item_size);
}

void* wrap_queue_pop(wrap_queue_t* q)
{
    if (wrap_queue_empty(q))
        return NULL;

    void* front = q->buffer + (q->head * q->item_size);
    q->head = (q->head + 1) % q->capacity;
    q->count--;

    return front;
}
