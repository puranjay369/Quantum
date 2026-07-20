#pragma once 
#include <pthread.h>
#include <stdlib.h>

#define CHANNEL_CAPACITY 64

typedef struct {
    int buffer[CHANNEL_CAPACITY];
    int head;
    int tail;
    int count;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} Channel;

void channel_init(Channel* ch);
void channel_send(Channel* ch, int value);
int  channel_recv(Channel* ch);
void channel_destroy(Channel* ch);
