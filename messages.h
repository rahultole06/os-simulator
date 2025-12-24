/*
* Created by Rahul Tole
* Course: CSCI 3120
*/

#ifndef MESSAGES_H
#define MESSAGES_H
#include <pthread.h>
#include "context.h"
#include "prio_q.h"

/* Holds a request */
typedef struct comm_request {
    context *proc;
    int partner_addr;
} comm_request_t;

/* Message facility */
typedef struct messages {
    prio_q_t *pending_sends;
    prio_q_t *pending_recvs;
    prio_q_t *completed_comms;

    pthread_mutex_t lock;
} messages_t;

/**
 * Initializes the message passing facility
 */
messages_t *messages_init();

/**
 * Destroys the message passing facility
 */
void messages_destroy(messages_t *m);

/**
 * Handles SEND request from a process
 */
void messages_send(messages_t *m, context *sender, int dest_addr);

/**
 * Handles RECV request from a process
 */
void messages_recv(messages_t *m, context *receiver, int source_addr);

/**
 * gets list of procs that finished comms in last tick
 */
prio_q_t *messages_get_completed(messages_t *m, int node_id);

#endif