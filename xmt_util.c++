#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include "xmt_util.h"
#include "ebp_client.h"

extern int ipc_fd_f;
namespace{
  // hold for book tag
  int isbn=-1, issue;

  pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
  pthread_mutex_t mp = PTHREAD_MUTEX_INITIALIZER;
};

bool wait_for_book(int a, int b){
  bool val = true;

  pthread_mutex_lock(&mp); 
  struct timespec to;
  struct timeval now;
  gettimeofday(&now, NULL);
  to.tv_sec = now.tv_sec + 5;
  to.tv_nsec = now.tv_usec * 1000;

  mag_tag tag={a,b};
  if(ipc_fd_f > 0) write(ipc_fd_f, &tag, sizeof(mag_tag));

  while ((a!=isbn) || (b!=issue)) {
    int err = pthread_cond_timedwait(&cv, &mp, &to); 
    if (err == ETIMEDOUT) 
    { /* timeout, do something */ 
      val = false;
      break; 
    } 
  } 
  pthread_mutex_unlock(&mp);
  return val;
}

void wake_for_book(int a, int b) {
  pthread_mutex_lock(&mp); 
  if ((a==isbn) && (b==issue)) 
    pthread_cond_signal(&cv); 
  pthread_mutex_unlock(&mp); 
}
