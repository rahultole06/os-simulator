/*
* Created by Rahul Tole by modifying code provided by Alex Brodsky (2023-05-07)
* Course: CSCI 3120
*/

#ifndef PROSIM_NODE_H
#define PROSIM_NODE_H
#include <pthread.h>
#include "context.h"
#include "prio_q.h"

/* Holds the finished queue & it's lock, as well as the  */
extern prio_q_t *finished;
extern pthread_mutex_t finished_lock;
extern pthread_mutex_t io_lock;

/* Represents a single node state */
typedef struct node_state {
    prio_q_t *blocked;      /* blocked queue */
    prio_q_t *ready;        /* ready queue */
    int time;               /* clock */
    int id;                 /* Node number */
    int quantum;            /* CPU quantum */
    int next_proc_id;
    int running_procs;
} node_state;

/* Initialize the node
 * @params:
 *   quantum: the CPU quantum to use in the situation
 *   id: The id of the node
 * @returns:
 *   returns node_state struct
 */
extern node_state *node_init(int cpu_quantum, int id);

/* Admit a process into the simulation
 * @params:
 *   proc: pointer to the program context of the process to be admitted
 * @returns:
 *   returns 1
 */
extern int process_admit(node_state *ns, context *proc);

/* Perform the simulation
 * @params:
 *   arg: The state of the node
 * @returns:
 *   returns NULL
 */
extern void *node_simulate(void *arg);

#endif //PROSIM_PROCESS_H
