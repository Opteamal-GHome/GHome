#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include "gthread.h"
#include "mere.h"
#include "gestion_regles.h"
#include "config.h"

//This file should be merged with gestion_regle.c, or the other way around...
static void engine_check(int signum){
  gsem_give(&sem);
}
void * startEngine(void * args){
  static struct sigaction sa;
  static timer_t timerId;
  sendLog(DEBUG,"engine thread started");
  
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = engine_check;
  sa.sa_flags = SA_RESTART;
  sigaction(SIGPROF, &sa, (struct sigaction *)0);

  /* set timer */ 
  struct sigevent evp = {
    .sigev_notify=SIGEV_SIGNAL,
    .sigev_signo=SIGPROF,
    .sigev_value.sival_int=42,
  };
  struct itimerspec timerValue = {
    .it_interval.tv_sec=engine_period,
    .it_interval.tv_nsec=0,
    .it_value.tv_sec=engine_period,
    .it_value.tv_nsec=0,
  };

  timer_create(CLOCK_REALTIME,&evp,&timerId);
  timer_settime(timerId,0,&timerValue,NULL);

  //wait for the semaphore :
  for (;;){
    gsem_take(&sem);
    //For test purposes (a sem_post is done on every sensor activity in sensorServer):
    sendLog(DEBUG,"Engine: checking rules");
    checkRules();
    sendLog(DEBUG,"Engine: rules checked");
  }
}
 
