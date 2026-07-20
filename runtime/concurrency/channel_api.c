#include "channel_api.h"

void channel_init(Channel* ch) {
    ch->head  = 0;
    ch->tail  = 0;
    ch->count = 0;
    pthread_mutex_init(&ch->lock, NULL);
    pthread_cond_init(&ch->not_empty, NULL);
    pthread_cond_init(&ch->not_full,  NULL);
}

void channel_send(Channel* ch, int value) {
    pthread_mutex_lock(&ch->lock);
    while (ch->count == CHANNEL_CAPACITY)
        pthread_cond_wait(&ch->not_full, &ch->lock);
    ch->buffer[ch->tail] = value;
    ch->tail = (ch->tail + 1) % CHANNEL_CAPACITY;
    ch->count++;
    pthread_cond_signal(&ch->not_empty);
    pthread_mutex_unlock(&ch->lock);
}

int channel_recv(Channel* ch) {
    pthread_mutex_lock(&ch->lock);
    while (ch->count == 0)
        pthread_cond_wait(&ch->not_empty, &ch->lock);
    int value = ch->buffer[ch->head];
    ch->head = (ch->head + 1) % CHANNEL_CAPACITY;
    ch->count--;
    pthread_cond_signal(&ch->not_full);
    pthread_mutex_unlock(&ch->lock);
    return value;
}

void channel_destroy(Channel* ch) {
    pthread_mutex_destroy(&ch->lock);
    pthread_cond_destroy(&ch->not_empty);
    pthread_cond_destroy(&ch->not_full);
}