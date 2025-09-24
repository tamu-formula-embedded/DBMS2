#include "queue.h"

#include <errno.h>

int queue_init(queue_t* q, void* buffer, size_t count, size_t elem_size)
{
    if (!q || count <= 0 || elem_size <= 0)
        return EINVAL;
    q->buffer = (uint8_t*)buffer;
    q->elem_size = elem_size;
    q->cap = count;
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    return 0;
}

void* queue_push(queue_t* q)
{
    if (queue_full(q)) return NULL;
    void* slot = q->buffer + (q->tail * q->elem_size);
    q->tail = (q->tail + 1) % q->cap;
    q->count++;
    return slot;
}

const void* queue_front(queue_t* q)
{
    if (queue_empty(q)) return NULL;
    return q->buffer + (q->head * q->elem_size);
}

bool queue_pop(queue_t* q)
{
    if (queue_empty(q)) return false;
    q->head = (q->head + 1) % q->cap;
    q->count--;
    return true;
}

bool queue_empty(queue_t* q)
{
    return q->count == 0;
}

bool queue_full(queue_t* q)
{
    return q->count == q->cap;
}
