/*
* Created by Rahul Tole
* Course: CSCI 3120
*/

#include <stdlib.h>
#include <assert.h>
#include "messages.h"

/* calculates proc address */
static int get_proc_addr(context *proc) {
    return (proc->node * 100) + proc->id;
}

/* Initialises message facility */
messages_t *messages_init() {
    messages_t *m = calloc(1, sizeof(messages_t));
    assert(m);

    pthread_mutex_init(&m->lock, NULL);
    m->pending_sends = prio_q_new();
    m->pending_recvs = prio_q_new();
    m->completed_comms = prio_q_new();
    
    return m;
}

/* frees requests on destruction */
static void clear_queue(prio_q_t *q) {
    while (!prio_q_empty(q)) {
        free(prio_q_remove(q));
    }
    free(q);
}

/* Destroys message facility */
void messages_destroy(messages_t *m) {
    pthread_mutex_destroy(&m->lock);
    clear_queue(m->pending_sends);
    clear_queue(m->pending_recvs);
    clear_queue(m->completed_comms);
    free(m);
}
/* sends message */
void messages_send(messages_t *m, context *sender, int dest_addr) {
    pthread_mutex_lock(&m->lock);

    comm_request_t *match = NULL;
    int sender_addr = get_proc_addr(sender);
    
    prio_q_t *temp_q = prio_q_new();

    /* Search for a receiver */
    while (!prio_q_empty(m->pending_recvs)) {
        comm_request_t *req = prio_q_remove(m->pending_recvs);

        if (req->partner_addr == sender_addr && dest_addr == get_proc_addr(req->proc)) {
            match = req;
            break; // Found a match
        } else {
            // No match, put in temp queue to restore
            prio_q_add(temp_q, req, 0);
        }
    }
    
    /* Restore removed receive requests */
    while (!prio_q_empty(temp_q)) {
        comm_request_t *req = prio_q_remove(temp_q);
        prio_q_add(m->pending_recvs, req, 0);
    }
    free(temp_q);

    if (match) {
        comm_request_t *sender_req = malloc(sizeof(comm_request_t));
        sender_req->proc = sender;
        sender_req->partner_addr = dest_addr;

        /* adds send and receive pairs to completed queue */
        prio_q_add(m->completed_comms, sender_req, sender_req->proc->id);
        prio_q_add(m->completed_comms, match, match->proc->id);
    } else {
        comm_request_t *sender_req = malloc(sizeof(comm_request_t));
        sender_req->proc = sender;
        sender_req->partner_addr = dest_addr;
        prio_q_add(m->pending_sends, sender_req, 0);
    }

    pthread_mutex_unlock(&m->lock);
}

void messages_recv(messages_t *m, context *receiver, int source_addr) {
    pthread_mutex_lock(&m->lock);

    comm_request_t *match = NULL;
    int receiver_addr = get_proc_addr(receiver);

    prio_q_t *temp_q = prio_q_new();

    /* Search for sender */
    while (!prio_q_empty(m->pending_sends)) {
        comm_request_t *req = prio_q_remove(m->pending_sends);
        if (req->partner_addr == receiver_addr && source_addr == get_proc_addr(req->proc)) {
            match = req;
            break;
        } else {
            prio_q_add(temp_q, req, 0);
        }
    }

    /* Restore removed send requests */
    while (!prio_q_empty(temp_q)) {
        comm_request_t *req = prio_q_remove(temp_q);
        prio_q_add(m->pending_sends, req, 0);
    }
    free(temp_q);

    if (match) {
        comm_request_t *receiver_req = malloc(sizeof(comm_request_t));
        receiver_req->proc = receiver;
        receiver_req->partner_addr = source_addr;
        prio_q_add(m->completed_comms, receiver_req, receiver_req->proc->id);
        prio_q_add(m->completed_comms, match, match->proc->id);
    } else {
        comm_request_t *receiver_req = malloc(sizeof(comm_request_t));
        receiver_req->proc = receiver;
        receiver_req->partner_addr = source_addr;
        prio_q_add(m->pending_recvs, receiver_req, 0);
    }

    pthread_mutex_unlock(&m->lock);
}

prio_q_t *messages_get_completed(messages_t *m, int node_id) {
    pthread_mutex_lock(&m->lock);

    prio_q_t *results_q = prio_q_new();
    prio_q_t *unclaimed_q = prio_q_new();

    while (!prio_q_empty(m->completed_comms)) {
        comm_request_t *req = prio_q_remove(m->completed_comms);
        if (req->proc->node == node_id) {
            prio_q_add(results_q, req->proc, req->proc->id);
            free(req);
        } else {
            prio_q_add(unclaimed_q, req, 0);
        }
    }

    while (!prio_q_empty(unclaimed_q)) {
        comm_request_t *req = prio_q_remove(unclaimed_q);
        prio_q_add(m->completed_comms, req, 0);
    }
    
    free(unclaimed_q);

    pthread_mutex_unlock(&m->lock);
    return results_q;
}