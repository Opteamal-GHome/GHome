//Initializer/logger/destructor in the ghome server which still has no name.
//Author(s) : Raphael C.
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
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
#define MQ_LOG_NAME "/mqLog"
static FILE * logFile=NULL;
static int verbose=0;

pthread_t sst; //sensorServerThread
struct msgData{
  int errnb;
  enum logLvl level;
  char data[140];
};

struct mlog{
  long mtype; //can be 0 for message or 1 for control
  struct msgData mtext;
};
static mqd_t msgLog; 

int sendLog(enum logLvl level, char * format,...) {
  struct mlog msg = {
    .mtype=INFO,
    .mtext.errnb=0,
    .mtext.level=level,
  };
  va_list args;
  va_start(args,format);
  vsprintf(msg.mtext.data,format,args);
  if (mq_send(msgLog,(void *)&msg, sizeof(struct mlog),0)==-1) {
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
  if (mq_send(msgLog,(void *)&msg, sizeof(struct mlog),1)==-1) {
    perror("[ERROR] sendLog");
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
  int ret = 0;
  printf("\n"); //this one is purely to avoid having the ^C polluting our output
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
  logMsg(LOG,"Exiting application");
  logMsg(DEBUG,"Canceling sst thread");
  ret=pthread_cancel(sst);
  if(ret==-1){
    logErr(WARNING,"Canceling failed",errno);
  } else {
    logMsg(DEBUG,"Joining sst thread");
    ret=pthread_join(sst,NULL);
    if(ret==-1){
      logErr(WARNING,"joining failed",errno);
    }
  }
  logMsg(DEBUG,"Removing message queue");
  if(mq_close(msgLog)){
    logErr(WARNING,"mq_close",errno);
  }
  if(mq_unlink(MQ_LOG_NAME)){
    logErr(DEBUG,"mq_unlink",errno);
  }
}
int main(int argc, char * argv[]) {
  //init 
  char buff[MSG_SIZE];
  //message box control :
  int stop=0;
  size_t msgSize;
  mode_t mqMode= S_IRWXO; //Allows everything for everyone
  struct mq_attr attrs = {
    .mq_maxmsg=10, //beyond 10 msgs one might need root acces
    .mq_msgsize=200,
  };
  //threads control :

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
  sigaction(SIGTERM,&act,NULL);

  //create log mailbox :
	if(mq_unlink(MQ_LOG_NAME)){
    logErr(DEBUG,"mq_unlink",errno);
  }
	msgLog = mq_open( MQ_LOG_NAME , /*O_NONBLOCK|*/O_EXCL|O_RDWR|O_CREAT,\
      mqMode, &attrs);
  if (msgLog==-1) {
    logErr(ERROR,"mq_open",errno);
    return -1;
  }
  snprintf(buff,MSG_SIZE,"Message box created with fd : %d",msgLog);
  logMsg(DEBUG,buff);

  //create various threads : 
  if (pthread_create(&sst,NULL,startSensorServer,NULL)!=0){
    logErr(ERROR,"pthread_create failed for sensorServer thread", errno);
    handler(0);
  }
  //Start the thread with the defaults arguments, using the startSensorServer 
  //function as entry point, with no arguments to this function.

  //Wait for messages to log :
  received.mtype=INFO;
  received.mtext.level=DEBUG;
  memcpy(received.mtext.data,"message received",18);
  if (mq_send(msgLog, (void *)&received, sizeof(struct mlog),0)==-1) {
    logErr(WARNING,"mq_send failed",errno);
  }
  for (stop=0; stop!=STOP;) {
    msgSize=mq_receive(msgLog, (void*)&received, 210, NULL);
    if (msgSize==-1) {
      logErr(DEBUG,"mq_receive",errno);
      if (errno==EINTR) {
        continue; //we were interupted because a signal was caught
      } else {
        stop=STOP;
        continue;
      }
    }
    switch (received.mtype) {
      case STOP :
        stop=STOP;
        break;
      case ERR :
        logErr(received.mtext.level, received.mtext.data,\
            received.mtext.errnb);
        break;
      case INFO :
        logMsg(received.mtext.level, received.mtext.data);
        break;
      default :
        logMsg(WARNING,"Received unexpected message");
    }
  }
  //Exit
  fclose(logFile);
  logMsg(DEBUG,"Closing file descriptors");
  exit(0);
}


