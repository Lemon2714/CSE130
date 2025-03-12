#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include "rwlock.h"

typedef struct rwlock {
    pthread_mutex_t mutex;
    pthread_cond_t write_complete;
    pthread_cond_t read_complete;
    int waiting_readers;
    int waiting_writers;
    int completed_readers;
    int max_readers;
    int active_readers;
    int active_writers;
    PRIORITY scheduling_priority;
} rwlock_t;

rwlock_t *rwlock_new(PRIORITY priority, uint32_t max_readers) {
    rwlock_t *rwlock = (rwlock_t *) malloc(sizeof(rwlock_t));
    if (rwlock == NULL) {
        exit(EXIT_FAILURE);
    }
    if (max_readers <= 0 && priority == N_WAY) {
        return NULL;
    }

    pthread_mutex_init(&rwlock->mutex, NULL);
    pthread_cond_init(&rwlock->read_complete, NULL);
    pthread_cond_init(&rwlock->write_complete, NULL);
    rwlock->active_readers = 0;
    rwlock->waiting_writers = 0;
    rwlock->max_readers = max_readers;
    rwlock->active_writers = 0;
    rwlock->completed_readers = 0;
    rwlock->scheduling_priority = priority;
    rwlock->waiting_readers = 0;
    return rwlock;
}

void rwlock_delete(rwlock_t **lock) {
    if (*lock == NULL || lock == NULL) {
        return;
    }
    rwlock_t *rwlock = *lock;
    pthread_mutex_lock(&rwlock->mutex);
    pthread_mutex_unlock(&rwlock->mutex);
    pthread_mutex_destroy(&rwlock->mutex);
    pthread_cond_destroy(&rwlock->write_complete);
    pthread_cond_destroy(&rwlock->read_complete);
    free(rwlock);
    *lock = NULL;
}

void reader_lock(rwlock_t *rw) {
    pthread_mutex_lock(&rw->mutex);
    rw->waiting_readers++;
    while (rw->active_writers > 0 || (rw->scheduling_priority == WRITERS && rw->waiting_writers > 0)
           || (rw->scheduling_priority == N_WAY && (rw->max_readers == 1 && rw->active_writers > 0))
           || (rw->scheduling_priority == N_WAY && rw->waiting_writers > 0
               && rw->completed_readers >= rw->max_readers)) {
        pthread_cond_wait(&rw->read_complete, &rw->mutex);
    }
    rw->active_readers++;
    rw->completed_readers++;
    rw->waiting_readers--;
    pthread_mutex_unlock(&rw->mutex);
}

void writer_lock(rwlock_t *rw) {
    pthread_mutex_lock(&rw->mutex);
    rw->waiting_writers++;
    while (rw->active_writers > 0 || rw->active_readers > 0
           || (rw->scheduling_priority == N_WAY && rw->waiting_readers > 0
               && rw->completed_readers < rw->max_readers)
           || (rw->waiting_readers > 0 && rw->scheduling_priority == READERS)) {
        pthread_cond_wait(&rw->write_complete, &rw->mutex);
    }
    rw->active_writers = 1;
    rw->completed_readers = 0;
    rw->waiting_writers--;
    pthread_mutex_unlock(&rw->mutex);
}

void reader_unlock(rwlock_t *rw) {
    pthread_mutex_lock(&rw->mutex);
    rw->active_readers--;
    if (rw->scheduling_priority == N_WAY) {
        if (rw->waiting_writers > 0) {
            if (rw->active_readers == 0 && rw->completed_readers >= rw->max_readers) {
                pthread_cond_signal(&rw->write_complete);
            } else if (rw->waiting_readers > 0) {
                pthread_cond_signal(&rw->read_complete);
            } else if (rw->active_readers == 0) {
                pthread_cond_signal(&rw->write_complete);
            }
        } else {
            pthread_cond_broadcast(&rw->read_complete);
        }
    } else if (rw->scheduling_priority == READERS) {
        if (rw->waiting_readers > 0) {
            pthread_cond_broadcast(&rw->read_complete);
        } else if (rw->active_readers == 0) {
            pthread_cond_broadcast(&rw->write_complete);
        }
    } else if (rw->scheduling_priority == WRITERS) {
        if (rw->waiting_writers == 0) {
            pthread_cond_broadcast(&rw->read_complete);
        } else if (rw->active_readers == 0) {
            pthread_cond_broadcast(&rw->write_complete);
        }
    }
    pthread_mutex_unlock(&rw->mutex);
}

void writer_unlock(rwlock_t *rw) {
    pthread_mutex_lock(&rw->mutex);
    rw->active_writers = 0;
    if (rw->scheduling_priority == N_WAY) {
        if (rw->waiting_writers > 0) {
            if (rw->active_readers == 0 && rw->completed_readers >= rw->max_readers) {
                pthread_cond_signal(&rw->write_complete);
            } else if (rw->waiting_readers > 0) {
                pthread_cond_signal(&rw->read_complete);
            } else if (rw->active_readers == 0) {
                pthread_cond_signal(&rw->write_complete);
            }
        } else {
            pthread_cond_broadcast(&rw->read_complete);
        }
    } else if (rw->scheduling_priority == READERS) {
        if (rw->waiting_readers > 0) {
            pthread_cond_broadcast(&rw->read_complete);
        } else if (rw->active_readers == 0) {
            pthread_cond_broadcast(&rw->write_complete);
        }
    } else if (rw->scheduling_priority == WRITERS) {
        if (rw->waiting_writers == 0) {
            pthread_cond_broadcast(&rw->read_complete);
        } else if (rw->active_readers == 0) {
            pthread_cond_broadcast(&rw->write_complete);
        }
    }
    pthread_mutex_unlock(&rw->mutex);
}
