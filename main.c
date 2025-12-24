/*
* Created by Rahul Tole
* Course: CSCI 3120
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "context.h"
#include "node.h"
#include "prio_q.h"
#include "barrier.h"
#include "messages.h"

/* Holds the barrier */
barrier_t *global_barrier = NULL;

/* Holds msg facility */
messages_t *global_messages = NULL;

int main() {
    int num_procs;
    int quantum;
    int num_nodes;

    /* Read in the header of the process description with minimal validation
     */
    if (scanf("%d %d %d", &num_procs, &quantum, &num_nodes) < 3) {
        fprintf(stderr, "Bad input, expecting number of process and quantum size\n");
        return -1;
    }

    /* Initialises the barrier */
    global_barrier = barrier_init(num_nodes);

    /* Inits msg facility */
    global_messages = messages_init();

    /* Holds a node states for each node */
    node_state *nodes = calloc(num_nodes, sizeof(node_state));
    for (int i = 0; i < num_nodes; i++){
        nodes[i] = *node_init(quantum, i+1);
    }
    
    /* We use an array of pointers to contexts to track the processes.
     */
    context **procs  = calloc(num_procs, sizeof(context *));

    /* Initialise final queue for finished processes */
    finished = prio_q_new();

    /* Load and admit each process, if an error occurs, we just give up.
     */
    for (int i = 0; i < num_procs; i++) {
        context *proc = context_load(stdin);
        procs[i] = proc;
        if (!proc) {
            fprintf(stderr, "Bad input, could not load program description\n");
            return -1;
        }
        process_admit(&nodes[proc->node - 1], proc);
    }

    /* We create a thread for each mode */
    pthread_t *threads = calloc(num_nodes, sizeof(pthread_t));
    for (int i = 0; i < num_nodes; i++) {
        pthread_create(&threads[i], NULL, node_simulate, &nodes[i]);
    }

    /* Wait for each thread to finish */
    for (int i = 0; i < num_nodes; i++) {
        pthread_join(threads[i], NULL);
    }

    /* Frees the barrier */
    barrier_destroy(global_barrier);

    /* Frees msg facility */
    messages_destroy(global_messages);

    /* Output the statistics for processes in order of amdmission.
     */
    while (!prio_q_empty(finished)) {
        context *proc;
        pthread_mutex_lock(&finished_lock);
        proc = prio_q_remove(finished);
        pthread_mutex_unlock(&finished_lock);
        context_stats(proc, stdout);
    }

    return 0;
}