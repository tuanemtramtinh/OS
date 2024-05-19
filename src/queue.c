#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
    if (q == NULL) return 1;
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
    /* TODO: put a new process to queue [q] */
    if (q->size < MAX_QUEUE_SIZE){
		q->proc[q->size] = proc;
		q->size++;
	}
}

struct pcb_t * dequeue(struct queue_t * q) {
    /* TODO: return a pcb whose prioprity is the highest
        * in the queue [q] and remember to remove it from q
        * */

    if (q->size != 0) {
        int max_prior= q->proc[0]->prio;
        int max_idx = 0;
        for (int i = 1; i < q->size; i++) {
            if (q->proc[i]->prio > max_prior) {
                max_prior = q->proc[i]->prio;
                max_idx = i;
            }
        }
        struct pcb_t* return_proc = q->proc[max_idx];
        for (int i = max_idx; i < q->size - 1; i++) {
                q->proc[i] = q->proc[i + 1];
        }
        q->proc[q->size - 1] = NULL;
        q->size--;

        return return_proc;
    }

    return NULL;
}

