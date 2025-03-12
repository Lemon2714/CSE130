#include <pthread.h>
#include <stdlib.h>
#include "queue.h"

struct queue {
    void **data;
    int front;
    int rear;
    int size;
    int capacity;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
};

queue_t *queue_new(int size) {
    queue_t *q = malloc(sizeof(queue_t));
    if (q == NULL)
        return NULL;
    q->data = malloc(size * sizeof(void *));
    if (q->data == NULL) {
        free(q);
        return NULL;
    }
    q->front = 0;
    q->rear = 0;
    q->size = 0;
    q->capacity = size;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
    return q;
}

void queue_delete(queue_t **q) {
    if (q && *q) {
        pthread_mutex_destroy(&(*q)->mutex);
        pthread_cond_destroy(&(*q)->not_empty);
        pthread_cond_destroy(&(*q)->not_full);
        free((*q)->data);
        free(*q);
        *q = NULL;
    }
}

bool queue_push(queue_t *q, void *elem) {
    if (!q)
        return false;

    pthread_mutex_lock(&q->mutex);
    while (q->size == q->capacity) {
        pthread_cond_wait(&q->not_full, &q->mutex);
    }

    q->data[q->rear] = elem;
    q->rear = (q->rear + 1) % q->capacity;
    q->size++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);

    return true;
}

bool queue_pop(queue_t *q, void **elem) {
    if (!q)
        return false;

    pthread_mutex_lock(&q->mutex);
    while (q->size == 0) {
        pthread_cond_wait(&q->not_empty, &q->mutex);
    }

    *elem = q->data[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mutex);

    return true;
}
