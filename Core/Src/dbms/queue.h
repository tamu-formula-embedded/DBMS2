//  
//  Copyright (c) Texas A&M University.
//  
#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stdbool.h>
#include <stddef.h>

#include "util.h"

// Fixed size circular queue

// typedef struct _CqCtx {
//     void** data;
//     uint32_t cap;
//     uint32_t front;
//     uint32_t rear;
//     uint32_t size;
// } CqCtx;

// Define a generic circular queue structure and functions for a given type
#define DEFINE_CQ(TYPE, NAME)                                                  \
    typedef struct _##NAME {                                                   \
        TYPE *buffer;                                                          \
        int32_t capacity;                                                        \
        int32_t front;                                                           \
        int32_t rear;                                                            \
        int32_t size;                                                            \
    } NAME;                                                                    \
                                                                               \
    static inline void NAME##Init(NAME *q, TYPE *buffer, int32_t capacity) {     \
        q->buffer = buffer;                                                    \
        q->capacity = capacity;                                                \
        q->front = q->rear = q->size = 0;                                      \
    }                                                                          \
                                                                               \
    static inline bool NAME##Enqueue(NAME *q, TYPE item) {                     \
        if (q->size == q->capacity)                                            \
            return false;                                                      \
        q->buffer[q->rear] = item;                                             \
        q->rear = (q->rear + 1) % q->capacity;                                 \
        q->size++;                                                             \
        return true;                                                           \
    }                                                                          \
                                                                               \
    static inline bool NAME##Dequeue(NAME *q, TYPE *out) {                     \
        if (q->size == 0)                                                      \
            return STATUS_FULL;                                                \
        *out = q->buffer[q->front];                                            \
        q->front = (q->front + 1) % q->capacity;                               \
        q->size--;                                                             \
        return STATUS_OK;                                                      \
    }                                                                          \
                                                                               \
    static inline bool NAME##Empty(const NAME *q) { return q->size == 0; }     \
                                                                               \
    static inline bool NAME##Full(const NAME *q) {                             \
        return q->size == q->capacity;                                         \
    }                                                                          \
                                                                               \
    static inline size_t NAME##Size(const NAME *q) { return q->size; }

#endif