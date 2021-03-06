//Thread containing the tcp server used in the communication with the sensor manager
//Author(s) : Rapahel C.
#include "mere.h"
#include "tcpserver.h"
#include "gestion_capteurs.h"
#include <endian.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>

struct frame {
	unsigned long long timestamp;
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

int sendOFrame(unsigned long long int stimestamp, int ssensorId, int sdata) {
	int i;
	char buff[20];
	uint64_t timestamp;
	int sensorId;
	int data;
	sendLog(DEBUG, "Sending O Frame");
	timestamp = htobe64(stimestamp);
	sensorId = htobe32(ssensorId);
	data = htobe32(sdata);
	memcpy(buff, &timestamp, sizeof(long long));
	buff[sizeof(long long)] = 'O';
	memcpy(&buff[9], &sensorId, sizeof(int));
	memcpy(buff + sizeof(long long) + 1 + sizeof(int), &data, sizeof(int));
	buff[17] = '\n';
	sendNetMsg(SENSORS, 18, buff);
	for (i = 0; i < 18; i++) {
		sendLog(DEBUG, "%hhx ", buff[i]);
	}
	return 0;
}

static void getSData(unsigned long long timestamp){
  //This frames are used to create or remove sensors, let's do that :
  struct sData received;
  struct DEVICE * new_dev=NULL;
  char data[6];
  int i;
  receive(socketSensorClient, (void *)data, 6);
  for (i=0; i<6; i++){
    //sendLog(DEBUG,"%x ", data[i]);
  }
  received.infoType=data[0];
  memcpy(&received.sensorId, &data[1], sizeof(received.sensorId));
  received.sensorType=data[5];
  received.sensorId=be32toh(received.sensorId);
  sendLog(DEBUG,"\ninfo type : '%c', sensor ID : %x, sensor type : '%c'",\
      received.infoType, received.sensorId, received.sensorType);
  switch (received.infoType){
  case 'A' :
  //look if the sensor already exist :
    new_dev=getMemDevice(received.sensorId);
    if(new_dev!=NULL){
      //update values in case it does :
      new_dev->type=received.sensorType;
      new_dev->timestamp=timestamp;
    }
  //look for a vacation in sensors table : 

    SENSORS_SAFE();
    for (i=0; i<nb_sensors; i++){
      if(sensors[i].id==0){
        sensors[i].id=received.sensorId;
        sensors[i].type=received.sensorType;
        sensors[i].timestamp=timestamp;
        sensors[i].role = getSensorRole(received.sensorType);
        sem_post(&sem);
        SENSORS_UNSAFE();
        return; 
      }
    }
    SENSORS_UNSAFE();
    sendLog(WARNING,"No more room in sensor table");
    break;
  case 'R' :
    removeMemDevice(received.sensorId);
    break;
  default :
    sendLog(WARNING,"Unknown info type in S frame decoding");
  }
} 

static void getOData() {
	struct oData received;
	receive(socketSensorClient, (void *) &received, 8);
	received.sensorId = be32toh(received.sensorId);
	received.data = be32toh(received.data);
	sendLog(DEBUG, "sensor ID : %d, sensor value : %d", received.sensorId,
			received.data);
	if ( setValue(received.sensorId, received.data) == TRUE )
	{
		transmitUpdate(received.sensorId, received.data);
		sem_post(&sem);
	}else{
		sendLog(WARNING, "Impossible to update sensor value (sensor ID : %d, unexistant)!!", received.sensorId);
	}
	
}

void * startSensorServer(void * args) {
	int ret;
	int socketServer = 0;
	struct frame received;
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, SIGTERM);
	sigaddset(&set, SIGINT);
	pthread_sigmask(SIG_UNBLOCK, &set, NULL);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
	//Those calls allow to uninstall the termination handler in this thread,
	//it is however a rather inelegant way to do it,
	//there must be some other way to achieve this.

	sendLog(DEBUG, "sensor reception thread started");
	socketSensorClient = 0;
	socketServer = initServer(sensors_listen_port);
	if (socketServer == -1) {
		return NULL;
	}
	sendLog(DEBUG, "sensorServer initialized, socket descriptor : %d",
			socketServer);
	for (;;) {
		sendLog(DEBUG, "sensorServer waiting for a client");
		socketSensorClient = waitClient(socketServer);
		if (socketSensorClient == -1) {
			return NULL;
		}
		sendLog(LOG, "Sensors client connected");
		//sendOFrame(4142,6666,1337);
		for (ret = 0; ret != -1;) {
			//Each frame is cut in 3 pieces, timestamp, type and data
			//The first two pieces have a fixed size of 9 bytes, let's get those.
			ret = receive(socketSensorClient, (char*) &received, 9);
			//In the previous call we don't need to worry about mem align for
			//sruct frame because of its definition :
			// - int 4 bytes,
			// - char 1 byte + 3 discarded bytes.
			// Convert into little endian :
			if (ret != -1) {
				received.timestamp = be64toh(received.timestamp);
				sendLog(
						DEBUG,
						"Received new frame,\n\ttimestamp : %llu,\n\ttype : '%c',",
						received.timestamp, received.type);
				switch (received.type) {
				case 'S':
					getSData(received.timestamp);
					break;
				case 'D':
				case 'O':
					getOData();
					break;
				default:
					sendLog(WARNING, "Unexpected frame type : %c",
							received.type);
				}
			}

		}
		sendLog(LOG, "Client deconected");
	}
	return NULL;
}
