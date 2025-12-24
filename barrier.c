/*
* Created by Rahul Tole
* Course: CSCI 3120
*/

#include <stdlib.h>
#include <assert.h>
#include "barrier.h"

/**
 * Initialises the barrier 
 * @params: 
 *  n: Number of threads using the barrier
 */
barrier_t *barrier_init(int n) {
    barrier_t *b = calloc(1, sizeof(barrier_t));
    assert(b);

    pthread_mutex_init(&b->lock, NULL);
    pthread_cond_init(&b->even_cond, NULL);
    pthread_cond_init(&b->odd_cond, NULL);

    b->max_threads = n;
    b->cur_threads = 0;
    b->generation = 0;

    return b;
}

/**
 * Makes a thread wait
 * @params:
 *  b: Barrier pointer
 */
void barrier_wait(barrier_t *b) {
    pthread_mutex_lock(&b->lock);

    /* Holds current generation to sync */
    int cur_gen = b->generation;

    b->cur_threads++;

    /* Checks if last in line */
    if (b->cur_threads < b->max_threads) {
        /* Waits for other threads */
        if (cur_gen % 2 == 0) {
            pthread_cond_wait(&b->even_cond, &b->lock);
        } else {
            pthread_cond_wait(&b->odd_cond, &b->lock);
        }
    } else {
        b->cur_threads = 0;
        b->generation++;
    }
    /* Signals waiting threads */
    if (cur_gen % 2 == 0) {
        pthread_cond_broadcast(&b->even_cond);
    } else {
        pthread_cond_broadcast(&b->odd_cond);
    }

    pthread_mutex_unlock(&b->lock);
}

/**
 * Removes thread from barrier
 * @params:
 *  b: Barrier pointer
 */
void barrier_done(barrier_t *b) {
    pthread_mutex_lock(&b->lock);

    b->max_threads--;
    if (b->cur_threads == b->max_threads) {
        int cur_gen = b->generation;
        b->cur_threads = 0;
        b->generation++;

        /* Signals waiting threads */
        if (cur_gen % 2 == 0) {
            pthread_cond_broadcast(&b->even_cond);
        } else {
            pthread_cond_broadcast(&b->odd_cond);
        }
    }

    pthread_mutex_unlock(&b->lock);
}

/**
 * Destroys the barrier
 * @params:
 *  b: Barrier pointer
 */
void barrier_destroy(barrier_t *b) {
    pthread_mutex_destroy(&b->lock);
    pthread_cond_destroy(&b->even_cond);
    pthread_cond_destroy(&b->odd_cond);
    free(b);
}
