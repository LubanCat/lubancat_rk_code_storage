#ifndef _RINGBUFF_H
#define _RINGBUFF_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef struct ring_buffer {
    void* buffer;
    size_t size;
    size_t element_size;
    size_t read_index;
    size_t write_index;
    size_t count;
    pthread_mutex_t mutex;
} ring_buffer_t;

ring_buffer_t* ring_buffer_init(size_t size, size_t element_size);
int ring_buffer_is_full(ring_buffer_t* rb);
int ring_buffer_is_empty(ring_buffer_t* rb);
int ring_buffer_write(ring_buffer_t* rb, const void* data);
int ring_buffer_read(ring_buffer_t* rb, void* data);
void ring_buffer_destroy(ring_buffer_t* rb);

#endif