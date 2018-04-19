#include "threadsafe-queue.h"

tsq *tsq_create() {
  tsq *q=malloc(sizeof(tsq));
  lock_t *lock=malloc(sizeof(pthread_mutex_t));
  lock_init(lock);
  q->head=0;
  q->tail=0;
  q->lock=lock;
  return q;
}

void tsq_push(tsq *q,int val) {
  lock_lock(q->lock);
  queuenode *n=malloc(sizeof(queuenode));
  n->next=0;
  n->value=val;
  if (q->head==0) {
    q->head=n;
    q->tail=n;
  } else {
    q->tail->next=n;
    q->tail=n;
  }
  lock_unlock(q->lock);
}

int tsq_pop_nonblock(tsq *q, int *val) {
  lock_lock(q->lock);
  if (q->head==0) {
    lock_unlock(q->lock);
    return 0;
  } 
  queuenode *todel=q->head;
  *val=q->head->value;
  q->head=q->head->next;
  free(todel);
  lock_unlock(q->lock);
  return 1;
}

void tsq_destroy(tsq *q) {
  lock_destroy(q->lock);
  free(q->lock);
  while (q->head) {
    queuenode *todel=q->head;
    q->head=q->head->next;
    free(todel);
  }
  free(q);
}