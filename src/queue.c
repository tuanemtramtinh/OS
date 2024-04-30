#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
    if (q == NULL) return 1;
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
    /* TODO: put a new process to queue [q] */
    if (q->size == MAX_QUEUE_SIZE) {
        printf("Queue is full\n");
        return;
    }

    int insertPos = 0; 
    while(insertPos < q->size && q->proc[insertPos]->prio <= proc->prio) insertPos += 1;

    for (int i = q->size; i > insertPos; i--){
        q->proc[i] = q->proc[i - 1];
    }

    q->proc[insertPos] = proc;
    q->size += 1;
}

struct pcb_t * dequeue(struct queue_t * q) {
    /* TODO: return a pcb whose prioprity is the highest
        * in the queue [q] and remember to remove it from q
        * */

    if (q->size == 0){
        printf("Queue is empty\n");
        return NULL;
    }

    struct pcb_t *proc = q->proc[0];
    for (int i = 0; i < q->size - 1; i++){
        q->proc[i] = q->proc[i + 1];
    }
    q->size-=1;

    return proc;

}

