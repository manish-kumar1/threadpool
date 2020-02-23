#ifndef _GNU_SOURCE
#define _GNU_SOURCE

#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include <string.h>

#include "platform/thread_config.h"

struct config* config_get_thread_config() {
  struct config* d = malloc(sizeof(struct config));
  if (d != NULL) {
    config_init(d);
  }
  return d;
}

void config_remove_thread_config(struct config* conf)
{
  if (conf != NULL) { free(conf); conf = NULL; }
}

void config_init(struct config* c) {
   c->policy_ = SCHED_OTHER;
   c->cancel_type_ = PTHREAD_CANCEL_ASYNCHRONOUS;
   c->cancel_state_ = PTHREAD_CANCEL_ENABLE;
   CPU_ZERO(&c->cpuset_);
   sigfillset(&c->blockedset_);
   memset(&c->sched_, 0, sizeof(struct sched_param));
   pthread_attr_init(&c->attribute_);
   c->sched_.sched_priority = 0;
}

int config_apply(pthread_t hndl, const struct config* conf) {
  if (hndl == 0) hndl = pthread_self();
  pthread_setcancelstate(conf->cancel_state_, NULL);
  pthread_setcanceltype(conf->cancel_type_, NULL);
  int ret = pthread_sigmask(SIG_BLOCK, &conf->blockedset_, NULL);
  if (CPU_COUNT(&conf->cpuset_) > 0)
    ret |= pthread_setaffinity_np(hndl, sizeof(cpu_set_t), &conf->cpuset_);

  ret |= pthread_setschedparam(hndl, conf->policy_, &conf->sched_);

  return ret;
}

int config_retrieve(pthread_t hndl, struct config* conf) {
  if (hndl == 0) hndl = pthread_self();
  int ret = pthread_getaffinity_np(hndl, sizeof(cpu_set_t), &conf->cpuset_);
  ret |= pthread_getschedparam(hndl, &conf->policy_, &conf->sched_);
  return ret;
}

struct config* config_set_priority(struct config* conf, int priority) {
  conf->sched_.sched_priority = priority;
  return conf;
}

struct config* config_set_policy(struct config* conf, int policy) {
  conf->policy_ = policy;
  return conf;
}

struct config* config_set_affinity(struct config* conf, int* cpuid, int num) {
  CPU_ZERO(&conf->cpuset_);
  for (int id = 0; id < num; ++id) { CPU_SET(cpuid[id], &conf->cpuset_); };
  return conf;
}

struct config* config_unblock_signals(struct config* conf, int* sig, int num) {
  sigset_t s;
  sigemptyset(&s);
  for (int i = 0; i < num; ++i) {
    sigaddset(&s, sig[i]);
  }
  pthread_sigmask(SIG_UNBLOCK, &s, &conf->blockedset_);

  return conf;
}

const sigset_t* config_get_sigset(struct config* conf) {
  pthread_sigmask(SIG_UNBLOCK, NULL, &conf->blockedset_);
  return &conf->blockedset_;
}

int config_cancel_thread(pthread_t hndl) {
  if (hndl == 0) hndl = pthread_self();
  return pthread_cancel(hndl);
}
#endif
