#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include "gthread.h"
#include "mere.h"
#include "gestion_regles.h"

void * startEngine(void * args){
  sigset_t set;

  sigemptyset(&set);
  sigaddset(&set, SIGTERM);
  sigaddset(&set, SIGINT);
  pthread_sigmask(SIG_UNBLOCK,&set,NULL);
  pthread_sigmask(SIG_BLOCK,&set,NULL);
  //Those calls allow to uninstall the termination handler in this thread,
  //it is however a rather inelegant way to do it,
  //there must be some other way to achieve this.
  sendLog(DEBUG,"engine thread started");

  //wait for the semaphore :
  for (;;){
    gsem_take(&sem);
    //For test purposes (a sem_post is done on every sensor activity in sensorServer):
    sendLog(DEBUG,"Engine: checking rules");
    checkRules();
    sendLog(DEBUG,"Engine: rules checked");
  }
}
 

