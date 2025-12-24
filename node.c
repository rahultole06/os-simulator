/*
* Created by Rahul Tole by modifying code provided by Alex Brodsky (2023-05-07)
* Course: CSCI 3120
*/

#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include "node.h"
#include "prio_q.h"
#include "barrier.h"
#include "messages.h"

prio_q_t *finished = NULL;                                  /* Finished queue */
pthread_mutex_t finished_lock = PTHREAD_MUTEX_INITIALIZER;  /* finished queue lock*/
pthread_mutex_t io_lock = PTHREAD_MUTEX_INITIALIZER;        /* lock used for printf */
extern barrier_t *global_barrier;
extern messages_t *global_messages;

enum {
    PROC_NEW = 0,
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,
    PROC_BLOCKED_SEND,
    PROC_BLOCKED_RECV,
    PROC_FINISHED
};

static char *states[] = {"new", "ready", "running", "blocked", "blocked (send)", "blocked (recv)", "finished"};

/* Initialize the simulation
 * @params:
 *   quantum: the CPU quantum to use in the situation
 * @returns:
 *   returns 1
 */
extern node_state *node_init(int cpu_quantum, int id) {
    node_state *ns = calloc(1, sizeof(node_state));
    assert(ns);

    ns->quantum = cpu_quantum;
    ns->blocked = prio_q_new();
    ns->ready = prio_q_new();
    ns->time = 0;
    ns->id = id;
    ns->next_proc_id = 1;
    ns->running_procs = 0;
    return ns;
}

/* Print state of process
 * @params:
 *   ns: node state
 *   proc: process' context
 * @returns:
 *   none
 */
static void print_process(node_state *ns, context *proc) {
    pthread_mutex_lock(&io_lock);
    printf("[%02d] %5.5d: process %d %s\n", ns->id, ns->time, proc->id, states[proc->state]);
    pthread_mutex_unlock(&io_lock);
}

/* Compute priority of process, depending on whether SJF or priority based scheduling is used
 * @params:
 *   proc: process' context
 * @returns:
 *   priority of process
 */
static int actual_priority(context *proc) {
    if (proc->priority < 0) {
        /* SJF means duration of current DOOP is the priority
         */
        return proc->duration;
    }
    return proc->priority;
}

/* Insert process into appropriate queue based on the primitive it is performing
 * @params:
 *   proc: process' context
 *   next_op: if true, current primitive is done, so move IP to next primitive.
 * @returns:
 *   none
 */
static void insert_in_queue(node_state *ns, context *proc, int next_op) {
    /* If current primitive is done, move to next
     */
    if (next_op) {
        context_next_op(proc);

        int op = context_cur_op(proc);
        if (op == OP_SEND || op == OP_RECV) {
            proc->duration = 1;
        } else {
            proc->duration = context_cur_duration(proc);
        }
    }

    int op = context_cur_op(proc);

    /* 3 cases:
     * 1. If DOOP, process goes into ready queue
     * 2. If BLOCK, process goes into blocked queue
     * 3. If HALT, process is not queued
     */
    if (op == OP_DOOP || op == OP_SEND || op == OP_RECV) {
        proc->state = PROC_READY;
        prio_q_add(ns->ready, proc, actual_priority(proc));
        proc->wait_count++;
        proc->enqueue_time = ns->time;
    } else if (op == OP_BLOCK) {
        /* Use the duration field of the process to store their wake-up time.
         */
        proc->state = PROC_BLOCKED;
        proc->duration += ns->time;
        prio_q_add(ns->blocked, proc, proc->duration);
    } else {
        proc->state = PROC_FINISHED;
        proc->finish_time = ns->time;
        ns->running_procs--;

        int priority = proc->finish_time * 100 * 100 + ns->id * 100 + proc->id;

        pthread_mutex_lock(&finished_lock);
        prio_q_add(finished, proc, priority);
        pthread_mutex_unlock(&finished_lock);
    }
    print_process(ns, proc);
}

/* Admit a process into the simulation
 * @params:
 *   proc: pointer to the program context of the process to be admitted
 * @returns:
 *   returns 1
 */
extern int process_admit(node_state *ns, context *proc) {
    /* Use a static variable to assign each process a unique process id.
     */
    proc->id = ns->next_proc_id++;
    proc->state = PROC_NEW;
    ns->running_procs++;
    print_process(ns, proc);
    insert_in_queue(ns, proc, 1);
    return 1;
}

/* Perform the simulation
 * @params:
 *   none
 * @returns:
 *   returns 1
 */
extern void *node_simulate(void *arg) {
    node_state *ns = arg;
    context *cur = NULL;
    int cpu_quantum = 0;

    /* Waits for all nodes to initialise */
    barrier_wait(global_barrier);

    /* We can only stop when all processes are in the finished state
     * no processes are readdy, running, or blocked
     */
    while(ns->running_procs > 0) {
        int preempt = 0;

        /* Unblock procs that finished their send/recvs */
        prio_q_t *unblocked_comms = messages_get_completed(global_messages, ns->id);
        while (!prio_q_empty(unblocked_comms)) {
            context *proc = prio_q_remove(unblocked_comms);
            insert_in_queue(ns, proc, 1);
            preempt |= cur != NULL && proc->state == PROC_READY &&
                    actual_priority(cur) > actual_priority(proc);
        }
        free(unblocked_comms);

        /* Step 1: Unblock processes
         * If any of the unblocked processes have higher priority than current running process
         *   we will need to preempt the current running process
         */
        while (!prio_q_empty(ns->blocked)) {
            /* We can stop ff process at head of queue should not be unblocked
             */
            context *proc = prio_q_peek(ns->blocked);
            if (proc->duration > ns->time) {
                break;
            }

            /* Move from blocked and reinsert into appropriate queue
             */
            prio_q_remove(ns->blocked);
            insert_in_queue(ns, proc, 1);

            /* preemption is necessary if a process is running, and it has lower priority than
             * a newly unblocked ready process.
             */
            preempt |= cur != NULL && proc->state == PROC_READY &&
                    actual_priority(cur) > actual_priority(proc);
        }

        /* Step 2: Update current running process
         */
        if (cur != NULL) {
            cur->duration--;
            cpu_quantum--;
            
            /* Process finished if primitive done, quantum finished or preempted */
            if (cur->duration == 0 || cpu_quantum == 0 || preempt) {
                // if primitive done
                if (cur->duration == 0) {
                    int op = context_cur_op(cur);
                    if (op == OP_SEND) {
                        cur->state = PROC_BLOCKED_SEND;
                        cur->send_count++;
                        print_process(ns, cur);
                        messages_send(global_messages, cur, context_cur_duration(cur));
                        cur = NULL;
                    } else if (op == OP_RECV) {
                        cur->state = PROC_BLOCKED_RECV;
                        cur->recv_count++;
                        print_process(ns, cur);
                        messages_recv(global_messages, cur, context_cur_duration(cur));
                        cur = NULL;
                    } else {
                        insert_in_queue(ns, cur, 1);
                        cur = NULL;
                    }
                } else { // quantum finished or preempted
                    insert_in_queue(ns, cur, 0);
                    cur = NULL;
                }
            }
        }
        barrier_wait(global_barrier);
        /* Step 3: Select next ready process to run if none are running
         * Be sure to keep track of how long it waited in the ready queue
         */
        if (cur == NULL && !prio_q_empty(ns->ready)) {
            cur = prio_q_remove(ns->ready);
            cur->wait_time += ns->time - cur->enqueue_time;
            cpu_quantum = ns->quantum;
            cur->state = PROC_RUNNING;
            print_process(ns, cur);
        }
        
        /* Waits for all nodes to sync clock */
        barrier_wait(global_barrier);

        /* next clock tick
         */
        ns->time++;
    }

    /* Thread simulation done */
    barrier_done(global_barrier);
    return NULL;
}
