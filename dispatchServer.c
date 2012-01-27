#include "mere.h"
#include "tcpserver.h"
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

void * startDispatchServer(void * args) {
  int socketSensors=0;
  int socketRest=0;
  sigset_t set;

  sigemptyset(&set);
  sigaddset(&set, SIGTERM);
  sigaddset(&set, SIGINT);
  pthread_sigmask(SIG_UNBLOCK,&set,NULL);
  pthread_sigmask(SIG_BLOCK,&set,NULL);
  //Those calls allow to uninstall the termination handler in this thread,
  //it is however a rather inelegant way to do it,
  //there must be some other way to achieve this.
  
  return NULL;
} 
