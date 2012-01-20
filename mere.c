//Initializer/logger/destructor
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include "tcpserver.h"
#include "mere.h"

#define ACCES 0660
#define LOG_SIZE 150
#define ERR_SIZE 30
#define MSG_SIZE 140
static FILE * logFile=NULL;
static int verbose=0;

struct msgData{
  int errnb;
  enum logLvl level;
  char data[140];
};

struct mlog{
  long mtype; //can be 0 for message or 1 for control
  struct msgData mtext;
};

static int msgLog; 

int sendLog(enum logLvl level, char * format,...) {
  struct mlog msg = {
    .mtype=INFO,
    .mtext.errnb=0,
    .mtext.level=level,
  };
  va_list args;
  va_start(args,format);
  vsprintf(msg.mtext.data,format,args);
  if (msgsnd(msgLog,(void *)&msg, MSG_SIZE,0)==-1) {
    perror("sendLog");
    va_end(args);
    return -1;
  }
  va_end(args);
  return 0;
}

int sendErr(enum logLvl level, char * info, int errnb) {
   struct mlog msg = {
    .mtype=ERR,
    .mtext.errnb=errnb,
    .mtext.level=level,
  };
  memcpy(msg.mtext.data,info,strlen(info));
  if (msgsnd(msgLog,(void *)&msg, MSG_SIZE,0)==-1) {
    perror("sendLog");
    return -1;
  }
  return 0;
}

static int logMsg(enum logLvl level, char * msg) {

  char buff[LOG_SIZE];
  switch (level){
    case LOG :
      snprintf(buff,LOG_SIZE,"%s %s\n","[LOG]",msg);
      break;
    case WARNING :
      snprintf(buff,LOG_SIZE,"%s %s\n","[WARN]",msg);
      break;
    case ERROR :
      snprintf(buff,LOG_SIZE,"%s %s\n","[ERROR]",msg);
      break;
    case DEBUG :
      snprintf(buff,LOG_SIZE,"%s %s\n","[DEBUG]",msg);
      break;
  }
  fprintf(logFile,"%s",buff);
  if (verbose>=1 || level==ERROR){
    printf("%s",buff);
  }
  return 0;
}
static int logErr(enum logLvl level, char * log, int err) {
  char errBuff[ERR_SIZE];
  char buff[MSG_SIZE];
  strerror_r(err,errBuff,ERR_SIZE);
  sprintf(buff,"%s : %s",log,errBuff);
  logMsg(level, buff);
  return 0;
}
void handler(int sigNb){
  //We've been asked to terminate  
  struct mlog msg = {
    .mtype=1,
  };
  switch (sigNb){
    case SIGINT:
      logMsg(LOG,"Received Ctrl-C, quiting GHome server");
      break;
    case SIGKILL:
      logMsg(LOG,"Received Sigkill, quitting GHome server");
      break;
  }
  //ask for application termination
  //test sendLog :
  sendLog(DEBUG,"Coucou lol %d, lolilol %s",15,"pouet");
  if (msgsnd(msgLog, (void *)&msg, 0, IPC_NOWAIT)==-1) {
    logErr(WARNING,"msgsnd failed in handler",errno);
  }
}
int main(int argc, char * argv[]) {
  //init 
  char buff[MSG_SIZE];
  //tcp server use :
//  int socketClient=0;
//  int socketServer=0;
//  int ret=0;
  //message box use :
  int stop=0;
  int msgSize;

  struct mlog received;
  struct sigaction act = {
    .sa_handler=handler,
    .sa_flags=0,
  };
  //temp : 
  verbose = 1;
  //ipcs IDs :
  msgLog = 0;

  printf("Starting application...\n");
  //create log file :
  logFile = fopen("./ghome.log","w");
  if (logFile==NULL) {
    perror("fopen ");
    return -1;
  }
  logMsg(LOG,"Log file created");
  //set up signals handlers :
  sigaction(SIGINT,&act,NULL);
  sigaction(SIGKILL,&act,NULL);
  //create log mailox :
	msgLog = msgget ( IPC_PRIVATE , ACCES | IPC_CREAT );
  if (msgLog==-1) {
    logErr(ERROR,"msgget",errno);
   return -1;
  }

  snprintf(buff,MSG_SIZE,"Message box created with id : %d",msgLog);
  logMsg(DEBUG,buff);
  //Wait for messages to log :
  for (stop=0; stop!=1;) {
    msgSize=msgrcv(msgLog, (void*)&received, MSG_SIZE, 0, MSG_NOERROR);
    if (msgSize==-1) {
      logErr(DEBUG,"msgrcv",errno);
      if (errno==EINTR) {
        continue; //we were interupted because a signal was caught
      } else {
        handler(0); 
        stop=STOP;
      }
    }
    switch (received.mtype) {
      case STOP :
        stop=STOP;
        break;
      case ERR :
        logErr(received.mtext.errnb, received.mtext.data,\
            received.mtext.errnb);
        break;
      case INFO :
        logMsg(received.mtext.level, received.mtext.data);
        break;
    }
  }
  //Exit
  logMsg(LOG,"Exiting application");
  logMsg(DEBUG,"Removing message queue");
	if(msgctl ( msgLog, IPC_RMID, 0 )){
    logErr(ERROR,"msgctl",errno);
  }
/*
  socketServer=initServer();  
  for (;;) {
    socketClient=waitClient(socketServer);
    for (; ret!=-1;){
      ret = receive(socketClient);
    }
    logMsg(LOG,"Client deconected");
  }*/
  logMsg(DEBUG,"Closing file descriptors");
  fclose(logFile);
  return 0;
}


