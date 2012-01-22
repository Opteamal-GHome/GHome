//Thread containing the tcp server used in the communication with the sensor manager
//Author(s) : Rapahel C.
#include "mere.h"
#include "tcpserver.h"
#include <unistd.h>
#include <pthread.h>

#define listen_port 80
struct frame {
  unsigned int timestamp;
  char type;
  char * data;
};
void * startSensorServer(void * args){
  int ret;
  int socketClient=0;
  int socketServer=0;
  struct frame received;

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
          break;
        case 'D' :
        case 'O' :
          break;
        default :
          sendLog(WARNING,"Unexpected frame type : %c",received.type);

      }
    }
    sendLog(LOG,"Client deconected");
  }
  return NULL;
}
