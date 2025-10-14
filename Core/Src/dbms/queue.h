/*
 * Ring-buffer queue which wraps around
 * a user allocated buffer.
 * 
 * Taken from Microsoft.
 * 
 * Copyright (C) Justus Languell
 */
 #ifndef _QUEUE_H_
 #define _QUEUE_H_
 
 #include <stdbool.h>
 #include <stddef.h>
 #include <stdint.h>
 
 /*
  * A fixed-size circular queue that stores generic items using a user-provided buffer.
  *
  * Does not support resizing or dynamic memory allocation.
  */
 typedef struct _queue_t
 {
     uint8_t* buffer;
     size_t elem_size;
     size_t cap;
     size_t head;
     size_t tail;
     size_t count;
 } queue_t;
 
 /*
  * Initialize the ring-buffer queue wrapping the given buffer.
  * 
  * sizeof(buffer) == elem_size * count
  */
 int queue_init(queue_t* q, void* buffer, size_t count, size_t elem_size);
 
 /*
  * Inserts a new item into the queue if there is space,
  * and returns a pointer to the location in memory.
  * 
  * Returns NULL if the queue is full.
  */
 void* queue_push(queue_t* q);
 
 /*
  * Returns a pointer to the element at the front of
  * the queue if the queue is not empty.
  * 
  * Returns NULL if the queue is empty.
  */
 const void* queue_front(queue_t* q);
 
 /*
  * Removes the element from the front of the queue
  * and returns true if the queue is not empty.
  * 
  * Returns false if the queue is empty.
  */
 bool queue_pop(queue_t* q);
 
 /*
  * Is the queue empty?
  */
 bool queue_empty(queue_t* q);
 
 /*
  * Is the queue full?
  */
 bool queue_full(queue_t* q);
 
 #endif
 