#ifndef THREADING_H
#define THREADING_H
#include <pthread.h>
#include <sched.h>

typedef pthread_t thread_t;
typedef pthread_mutex_t lock_t;

void lock_init(lock_t *lock);
void lock_lock(lock_t *lock);
void lock_unlock(lock_t *lock);
void lock_destroy(lock_t *lock);

void thread_create(thread_t *thread, void*(*fun)(void*),void*arg);
void thread_join(thread_t thread);
void thread_yield(void);

#endif