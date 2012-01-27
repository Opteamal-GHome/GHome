//Thread containing the tcp server used in the communication with the sensor manager
//Author(s) : Rapahel C.
#include "mere.h"
#include "tcpserver.h"
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#define listen_port 80

struct frame {
  unsigned int timestamp;
  char type;
  char * data;
};
struct sData {
  char infoType;
  int sensorId;
  char sensorType;
};
struct oData {
  int sensorId;
  int data; 
};

static int socketClient=0;

static void getSData(){
  struct sData received;
  receive(socketClient, (void *)&received, 6);
  sendLog(DEBUG,"info type : '%c', sensor ID : %d, sensor type : '%c'",\
      received.infoType, received.sensorId, received.sensorType);
} 

static void getOData(){
  struct oData received;
  receive(socketClient, (void *)&received, 8);
  sendLog(DEBUG,"sensor ID : %d, sensor value : %d",\
      received.sensorId, received.data);
}

void * startSensorServer(void * args){
  int ret;
  int socketServer=0;
  struct frame received;
  sigset_t set;

  sigemptyset(&set);
  sigaddset(&set, SIGTERM);
  sigaddset(&set, SIGINT);
  pthread_sigmask(SIG_UNBLOCK,&set,NULL);
  pthread_sigmask(SIG_BLOCK,&set,NULL);
  //Those calls allow to uninstall the termination handler in this thread,
  //it is however a rather inelegant way to do it,
  //there must be some other way to achieve this.

  sendLog(DEBUG,"sensorServer started");
  socketServer=initServer(listen_port);
  if (socketServer==-1) {
    return NULL;
  }
  sendLog(DEBUG,"sensorServer initialized, socket descriptor : %d",socketServer);
  for (;;) {
    sendLog(DEBUG,"sensorServer waiting for a client");
    socketClient=waitClient(socketServer);
    if (socketClient==-1) {
      return NULL;
    }
    for (ret=0; ret!=-1;){
      //Each frame is cut in 3 pieces, timestamp, type and data
      //The first two pieces have a fixed size of 5 bytes, let's get those.
      ret = receive(socketClient, (void*)&received ,5);
      sendLog(DEBUG,"Received new frame,\n\ttimestamp : %d,\n\ttype : '%c',",\
          received.timestamp, received.type);
      switch (received.type) {
        case 'S' :
          getSData();
          break;
        case 'D' :
        case 'O' :
          getOData();
          break;
        default :
          sendLog(WARNING,"Unexpected frame type : %c",received.type);
      }
    }
    sendLog(LOG,"Client deconected");
  }
  return NULL;
}
