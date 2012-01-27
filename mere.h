#ifndef __MERE_H
#define __MERE_H

#include <mqueue.h>
#include "gestion_capteurs.h"

#define STOP 1
#define ERR 2
#define INFO 3
#define REST 1
#define SENSORS 2

int socketSensorClient;
int socketRestClient;
enum logLvl{
  LOG,
  WARNING,
  ERROR,
  DEBUG
};
struct netMsg {
  int dest; //Can be either REST or SENSORS
  int msgSize;
  char * data;
};
int sendNetMsg(int destination, int len, char * msg);

void * startSensorServer(void *); //Implemented in sensorServer.c
void * startDispatchServer(void * args); //Implemented in dispatchServer.c
void * startRestRcv(void * args); //implemented in restRcv.c

int sendLog(enum logLvl level, char * format,...);
//This function allows a thread to ask for a information to be logged
//in a printf-like style. The 'level' argument is used to describe the 
//type of the message to be printed. In a future release, a run time 
//option may be used to specified whether the WARNING and DEBUG messages will
//be printed to the screen.
//Whatever his type is, a message will always get printed in the 
//ghome.log file.

int sendErr(enum logLvl level, char * info, int errnb);
//This function allows a thread to log an errno in a human readable form
//(much like perror). The 'level' argument has the same utility as in the 
//previous function.
#endif
