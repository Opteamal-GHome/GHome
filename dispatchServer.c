#include "mere.h"
#include "tcpserver.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <mqueue.h>
#include <errno.h>
#include "ipcs.h"

static mqd_t dispatchReq;
int sendNetMsg(int destination, int len, char * msg){
  struct netMsg newMsg={
    .dest=destination,
    .msgSize=len,
  };
  if (dispatchReq==0) {
    sendLog(DEBUG,"dispatch mq not opened yet"); //I don't think this is actually possible.
    return -1;
  }
  //Allocate enough memory to copy the msg,
  //so that the original buffer can be gfreed after this function call.
  //this memory will be gfreed once the message is read.
  newMsg.data=gmalloc(len); 
	memcpy(newMsg.data, msg, len);
  memcpy(newMsg.data,msg,len);
  if (gmq_send(dispatchReq,(void*)&newMsg,sizeof(struct netMsg),0)){
    sendErr(WARNING,"mq_send dispatch request failed",errno);
    return -1;
  }
  return 0;
}

void * startDispatchServer(void * args) {
	int stop = 0;
	int msgSize;
	struct netMsg received;
	sigset_t set;
  int stop=0;
  int msgSize;
  struct netMsg received;
  sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, SIGTERM);
	sigaddset(&set, SIGINT);
	pthread_sigmask(SIG_UNBLOCK, &set, NULL);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
	//Those calls allow to uninstall the termination handler in this thread,
	//it is however a rather inelegant way to do it,
	//there must be some other way to achieve this.

	//get the message queue id :
	sendLog(DEBUG, "dispatch thread started");
	dispatchReq = *(mqd_t*) args;
	for (stop = 0; stop != STOP;) {
  sigemptyset(&set);
  sigaddset(&set, SIGTERM);
  sigaddset(&set, SIGINT);
  pthread_sigmask(SIG_UNBLOCK,&set,NULL);
  pthread_sigmask(SIG_BLOCK,&set,NULL);
  //Those calls allow to uninstall the termination handler in this thread,
  //it is however a rather inelegant way to do it,
  //there must be some other way to achieve this.
  
  //get the message queue id :
  sendLog(DEBUG,"dispatch thread started");
  dispatchReq=*(mqd_t*)args;
  for (stop=0; stop!=STOP;) {
    msgSize=gmq_receive(dispatchReq, (void*)&received, sizeof(struct netMsg), NULL);
				sizeof(struct netMsg), NULL);
		if (msgSize == -1) {
			sendErr(DEBUG, "dispatch mq_receive", errno);
			if (errno == EINTR) {
				continue; //we were interupted because a siqnal was caught
			} else {
				stop = STOP; //There is probably some other crapy error, exit.
				continue;
			}
		}
		switch (received.dest) {
		case REST:
			transmit(socketRestClient, received.data, received.msgSize);
			break;
		case SENSORS:
			sendLog(DEBUG, "Sending message to sensors");
			transmit(socketSensorClient, received.data, received.msgSize);
			break;
		case RESTUP:
			//Init rest Server Socket
			if(startUpdateSender() != -1){
				sendLog(DEBUG, "Sending message to sensors up");
				transmit(socketRestServer, received.data, received.msgSize);
				//Destroy socket for sender
				/* fermeture de la connection */
				shutdown(socketRestServer, 2);
				close(socketRestServer);
			}	
			break;
		default:
			sendLog(WARNING,
					"dispatch msg, unexpected message dest received : %d",
					received.dest);
		}
    gfree(received.data); //Allocated in sendNetMsg()
	}
	return NULL;
}
