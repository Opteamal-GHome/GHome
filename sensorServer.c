//Thread containing the tcp server used in the communication with the sensor manager
//Author(s) : Rapahel C.
#include "mere.h"
#include "tcpserver.h"
#include "gestion_capteurs.h"
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>

#define listen_port 80

struct frame {
  unsigned int timestamp;
  char type;
};
struct sData {
  char infoType;
  unsigned int sensorId;
  char sensorType;
};
struct oData {
  int sensorId;
  int data; 
};

static void getSData(unsigned int timestamp){
  //This frames are used to create or remove sensors, let's do that :
  struct sData received;
  char data[6];
  int i;
  receive(socketSensorClient, (void *)data, 6);
  for (i=0; i<6; i++){
    sendLog(DEBUG,"%x ", data[i]);
  }
  received.infoType=data[0];
  memcpy(&received.sensorId, &data[1], sizeof(received.sensorId));
  received.sensorType=data[5];
  sendLog(DEBUG,"\ninfo type : '%c', sensor ID : %x, sensor type : '%c'",\
      received.infoType, received.sensorId, received.sensorType);
  switch (received.infoType){
  case 'A' :
  //look for a vacation in sensors table : 
    for (i=0; i<NB_SENSORS; i++){
      if(sensors[i].id==0){
        sensors[i].id=received.sensorId;
        sensors[i].type=received.sensorType;
        sensors[i].timestamp=timestamp;
        return; 
      }
    }
    sendLog(WARNING,"No more room in sensor table");
    break;
  case 'R' :
    removeMemDevice(received.sensorId);
    break;
  default :
    sendLog(WARNING,"Unknown info type in S frame decoding");
  }
} 

static void getOData(){
  struct oData received;
  receive(socketSensorClient, (void *)&received, 8);
  sendLog(DEBUG,"sensor ID : %d, sensor value : %d",\
      received.sensorId, received.data);
  setValue(received.sensorId, received.data);
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

  sendLog(DEBUG,"sensor reception thread started");
  socketSensorClient=0;
  socketServer=initServer(listen_port);
  if (socketServer==-1) {
    return NULL;
  }
  sendLog(DEBUG,"sensorServer initialized, socket descriptor : %d",socketServer);
  for (;;) {
    sendLog(DEBUG,"sensorServer waiting for a client");
    socketSensorClient=waitClient(socketServer);
    if (socketSensorClient==-1) {
      return NULL;
    }
    sendLog(LOG, "Sensors client connected");
    for (ret=0; ret!=-1;){
      //Each frame is cut in 3 pieces, timestamp, type and data
      //The first two pieces have a fixed size of 5 bytes, let's get those.
      ret = receive(socketSensorClient, (void*)&received ,5);
      //In the previous call we don't need to worry about mem align for
      //sruct frame because of its definition : 
      // - int 4 bytes, 
      // - char 1 byte + 3 discarded bytes.
      //This wouldn't be true in a 64bit arch since word size would 
      //then grow to 8 bytes.
      sendLog(DEBUG,"Received new frame,\n\ttimestamp : %ud,\n\ttype : '%c',",\
          received.timestamp, received.type);
      sendNetMsg(SENSORS,9,"coucou!\n"); //test transmit function
      sem_post(&sem);
      switch (received.type) {
        case 'S' :
          getSData(received.timestamp);
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
