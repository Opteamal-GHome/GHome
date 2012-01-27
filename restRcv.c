#include "mere.h"
#include "tcpserver.h"
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

#define listen_port 447
static int socketClient=0;
void * startRestRcv(void * args){
  sigset_t set;
  int socketServer=0;
  char received[10]; //thos has to be changed to something real^^

  sigemptyset(&set);
  sigaddset(&set, SIGTERM);
  sigaddset(&set, SIGINT);
  pthread_sigmask(SIG_UNBLOCK,&set,NULL);
  pthread_sigmask(SIG_BLOCK,&set,NULL);
  //Those calls allow to uninstall the termination handler in this thread,
  //it is however a rather inelegant way to do it,
  //there must be some other way to achieve this.

  sendLog(DEBUG,"rest messages reception started");
  socketServer=initServer(listen_port);
  if (socketServer==-1) {
    return NULL;
  }
  sendLog(DEBUG,"restServer initialized, socket descriptor : %d",socketServer);
  socketClient=waitClient(socketServer);
  if (socketClient==-1) {
    sendErr(WARNING,"restServer, wait failed",errno);
    return NULL;
  }
  sendLog(DEBUG,"sensorServer client client connected");
  receive(socketClient, (void*)&received ,5);

  return NULL;
}
