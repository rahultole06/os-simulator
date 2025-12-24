/*
* Created by Rahul Tole
* Course: CSCI 3120
*/

#ifndef BARRIER_H
#define BARRIER_H
#include <pthread.h>

/* Holds the barrier */
typedef struct barrier {
    pthread_mutex_t lock;
    pthread_cond_t even_cond;
    pthread_cond_t odd_cond;

    int max_threads;
    int cur_threads;
    int generation; // keeps track of odd/even for conditions
} barrier_t;

/**
 * Initialises the barrier 
 * @params: 
 *  n: Number of threads using the barrier
 */
barrier_t *barrier_init(int n);

/**
 * Makes a thread wait
 * @params:
 *  b: Barrier pointer
 */
void barrier_wait(barrier_t *b);

/**
 * Removes thread from barrier
 * @params:
 *  b: Barrier pointer
 */
void barrier_done(barrier_t *b);

/**
 * Destroys the barrier
 * @params:
 *  b: Barrier pointer
 */
void barrier_destroy(barrier_t *b);

#endif