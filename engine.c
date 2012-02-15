#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include "gthread.h"
#include "mere.h"
#include "gestion_regles.h"

//This file should be merged with gestion_regle.c, or the other way around...
void * startEngine(void * args){
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
 
