#include "threading.h"

void lock_init(lock_t *lock) {
  pthread_mutex_init(lock, NULL);
}

void lock_lock(lock_t *lock) {
  pthread_mutex_lock(lock);
}

void lock_unlock(lock_t *lock) {
  pthread_mutex_unlock(lock);
}

void lock_destroy(lock_t *lock) {
  pthread_mutex_destroy(lock);
}

void thread_create(thread_t *thread, void*(*fun)(void*),void*arg) {
  pthread_create(thread,NULL,fun,arg);
}

void thread_join(thread_t thread) {
  pthread_join(thread,NULL);
}

void thread_yield(void) {
  sched_yield();
}
