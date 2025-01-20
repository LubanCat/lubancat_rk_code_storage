#include "ringbuff.h"

ring_buffer_t* ring_buffer_init(size_t size, size_t element_size) 
{
    ring_buffer_t* rb = (ring_buffer_t*)malloc(sizeof(ring_buffer_t));
    if (rb == NULL) 
        return NULL;
    
    rb->size = size;
    rb->element_size = element_size;
    rb->buffer = malloc(size * element_size);
    if (rb->buffer == NULL) 
    {
        free(rb);
        return NULL;
    }
    rb->read_index = 0;
    rb->write_index = 0;
    rb->count = 0;
    if (pthread_mutex_init(&rb->mutex, NULL)!= 0) 
    {
        free(rb->buffer);
        free(rb);
        return NULL;
    }
    return rb;
}

int ring_buffer_is_full(ring_buffer_t* rb) 
{
    return rb->count == rb->size;
}

int ring_buffer_is_empty(ring_buffer_t* rb) 
{
    return rb->count == 0;
}

int ring_buffer_write(ring_buffer_t* rb, const void* data) 
{
    pthread_mutex_lock(&rb->mutex);
    if (ring_buffer_is_full(rb)) 
    {
        pthread_mutex_unlock(&rb->mutex);
        return -1; // 缓冲区已满
    }
    memcpy((char*)rb->buffer + rb->write_index * rb->element_size, data, rb->element_size);
    rb->write_index = (rb->write_index + 1) % rb->size;
    rb->count++;
    pthread_mutex_unlock(&rb->mutex);
    return 0;
}

int ring_buffer_read(ring_buffer_t* rb, void* data) 
{
    pthread_mutex_lock(&rb->mutex);
    if (ring_buffer_is_empty(rb)) 
    {
        pthread_mutex_unlock(&rb->mutex);
        return -1; // 缓冲区已空
    }
    memcpy(data, (char*)rb->buffer + rb->read_index * rb->element_size, rb->element_size);
    rb->read_index = (rb->read_index + 1) % rb->size;
    rb->count--;
    pthread_mutex_unlock(&rb->mutex);
    return 0;
}

void ring_buffer_destroy(ring_buffer_t* rb) 
{
    pthread_mutex_destroy(&rb->mutex);
    free(rb->buffer);
    free(rb);
}