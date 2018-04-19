#ifndef THREADSAFE_QUEUE_H
#define THREADSAFE_QUEUE_H
#include "threading.h"
#include <stdlib.h>
typedef struct squeuenode {
  struct squeuenode* next;
  int value;
} queuenode;

typedef struct {
  queuenode* head;
  queuenode* tail;
  lock_t *lock;
} tsq;

tsq *tsq_create();
void tsq_push(tsq *q,int val);
void tsq_pop_block(tsq *q,int *val);
int tsq_pop_nonblock(tsq *q,int *val);
void tsq_destroy(tsq *q);
#endif