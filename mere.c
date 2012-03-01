//Initializer/logger/destructor in the ghome server which still has no name.
//Author(s) : Raphael C.
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <mqueue.h>
#include <string.h>
#include <errno.h>
#include <gthread.h>
#include <gipcs.h>
#include <stdarg.h>
#include <unistd.h>
#include "tcpserver.h"
#include "mere.h"
#include "config.h"
#include "gestion_capteurs.h"

#define ACCES 0660
#define LOG_SIZE 150
#define ERR_SIZE 30
#define MSG_SIZE 130
#define MQ_LOG_NAME "/mqLog"
#define MQ_DISPATCH_NAME "/mqDis"
#define STACKSZ 65536 //64kB

#define RED(a) "\x1b[31m"a"\x1b[0m"
#define MAGENTA(a) "\x1b[35m"a"\x1b[0m"
#define BLUE(a) "\x1b[34m"a"\x1b[0m"
#define CYAN(a) "\x1b[36m"a"\x1b[0m"
#define GREEN(a) "\x1b[32m"a"\x1b[0m"
#define BOLD(a) "\x1b[1m"a"\x1b[0m"
#define UNDERLINED(a) "\x1b[4m"a"\x1b[0m"

static FILE * logFile = NULL;
static int verbose = 0;

gthread_t sst; //sensorServerThread
gthread_t dst; //dispatchServerThread
gthread_t rrt; //rest rcv Thread
gthread_t iet; //inference engine Thread
struct msgData {
	int errnb;
	enum logLvl level;
	char data[MSG_SIZE];
};

struct mlog {
	long mtype; //can be 0 for message or 1 for control
	struct msgData mtext;
};
static mqd_t dispatchReq; //message queue to send messages to the outside world 
static mqd_t msgLog;

int sendLog(enum logLvl level, char * format, ...) {
	struct mlog msg = { .mtype = INFO, .mtext.errnb = 0, .mtext.level = level, };
	va_list args;
	va_start(args, format);
	vsprintf(msg.mtext.data, format, args);
  if (gmq_send(msgLog,(void *)&msg, sizeof(struct mlog),0)==-1) {
		perror("sendLog");
		va_end(args);
		return -1;
	}
  va_end(args);
	return 0;
}

int sendErr(enum logLvl level, char * info, int errnb) {
	struct mlog msg =
			{ .mtype = ERR, .mtext.errnb = errnb, .mtext.level = level, };
	memcpy(msg.mtext.data, info, strlen(info));
  if (gmq_send(msgLog,(void *)&msg, sizeof(struct mlog),1)==-1) {
		perror(BOLD(RED("[ERROR] sendLog")));
		return -1;
	}
	return 0;
}
static int ckprintf(int level, char * format, ...){
  int ret=0;
  va_list args;
  if ( level < verbose + 2){ //+2 so that we always display log and warning msgs
    va_start(args,format);
    ret=vprintf(format,args);
    va_end(args);
  }
  return ret; 
}
static int logMsg(enum logLvl level, char * msg) {

	switch (level) {
	case LOG:
		fprintf(logFile, "[LOG] ");
		ckprintf(level,BOLD(GREEN("[LOG] ")));
		break;
	case WARNING:
		fprintf(logFile, "[WARN] ");
		ckprintf(level,BOLD(CYAN("[WARN] ")));
		break;
	case ERROR:
		fprintf(logFile, "[ERROR] ");
		ckprintf(level,BOLD(RED("[ERROR] ")));
		break;
	case DEBUG:
		fprintf(logFile, "[DEBUG] ");
		ckprintf(level,BOLD(BLUE("[DEBUG] ")));
		break;
	}
	fprintf(logFile, "%s\n", msg);
  ckprintf(level,"%s\n", msg);
	return 0;
}
static int logErr(enum logLvl level, char * log, int err) {
	char errBuff[ERR_SIZE];
	char buff[MSG_SIZE];
	strerror_r(err, errBuff, ERR_SIZE);
	sprintf(buff, "%s : %s", log, errBuff);
	logMsg(level, buff);
	return 0;
}
void handler(int sigNb) {
	//We've been asked to terminate
  //int ret = 0;
	printf("\n"); //this one is purely to avoid having the ^C polluting our output
	switch (sigNb) {
	case SIGINT:
		logMsg(LOG, "Received Ctrl-C, quiting GHome server");
		break;
	case SIGKILL:
		logMsg(LOG, "Received Sigkill, quitting GHome server");
		break;
	}
	//ask for application termination
	logMsg(LOG, "Exiting application");
	logMsg(DEBUG, "Canceling threads");

	gthread_cancel(rrt);
	gthread_cancel(dst);
	gthread_cancel(iet);

	logMsg(DEBUG, "Removing message queues");
	if (mq_close(dispatchReq)) {
		logErr(WARNING, "mq_close dispatchReq", errno);
	}
	if (mq_unlink(MQ_DISPATCH_NAME)) {
		logErr(DEBUG, "mq_unlink", errno);
	}
	if (mq_close(msgLog)) {
		logErr(WARNING, "mq_close msgLog", errno);
	}
	if (mq_unlink(MQ_LOG_NAME)) {
		logErr(DEBUG, "mq_unlink", errno);
	}
  fclose(logFile);
  logMsg(DEBUG,"Closing file descriptors");
  exit(0);
}

static int parseArgs(int argc, char * argv[]){
  int i=1;
  int j=0;

  switch (argc){
    case 1:
      return 0;
    case 2:
      break;
    default :
      return -1;
  }

  if (argv[i][0]!='-'){
    return -1;
  }

  for (j=1; j<4; j++){
    switch (argv[i][j]){
      case '\0':
        return j>1 ? 0 : -1;
        break;
      case 'v':
        verbose++;
        break;
      default :
        return -1;
        break;
    }
  } 

  return argc>1 ? -1 : 0;
}

int main(int argc, char * argv[]) {
	//init
	char buff[MSG_SIZE];
	//message box control :
	int stop = 0;
	size_t msgSize;
	mode_t mqMode = S_IRWXO; //Allows everything for everyone
	//TODO : change the mode to user.
	struct mq_attr attrs = { 
    .mq_maxmsg = 90, //beyond 10 msgs one might need root acces
  };
	//threads control :

	struct mlog received;
	struct sigaction act = { .sa_handler=handler, .sa_flags = 0, };

  if (parseArgs(argc, argv)!=0){
    printf(BOLD(CYAN("[WARN]"))" Bad option.\n");
    verbose=0;
  }
	//ipcs IDs :
	msgLog = 0;

	printf("Starting application...\n");
	//create log file :
	logFile = fopen("./ghome.log", "w");
	if (logFile == NULL) {
		perror("fopen ");
		return -1;
	}
	logMsg(LOG, "Log file created");
	//set up signals handlers :
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);

  //Initialize gthread environment :
  gthread_init(NULL);

  //destroy olds log mailboxes :
	if (mq_unlink(MQ_LOG_NAME)) {
		logErr(DEBUG, "mq_unlink log", errno);
	}
	if (mq_unlink(MQ_DISPATCH_NAME)) {
		logErr(DEBUG, "mq_unlink dispatch", errno);
	}
	attrs.mq_msgsize = 200;
	msgLog = gmq_open( MQ_LOG_NAME, O_EXCL|O_RDWR|O_CREAT,\
			mqMode, &attrs);
	if (msgLog == -1) {
		logErr(ERROR, "mq_open", errno);
		return -1;
	}
	snprintf(buff, MSG_SIZE, "Message box created with fd : %d", msgLog);
	logMsg(DEBUG, buff);

  logMsg(LOG,"Loading ghome.conf...");
  if (load_config()==-1){
    logMsg(WARNING,"No config file could be found, using default values");
  } else {
    logMsg(LOG,"Config file loaded");
  }

	attrs.mq_msgsize = sizeof(struct netMsg);
	//Create a message queue to receive dispatch request :
	dispatchReq = gmq_open( MQ_DISPATCH_NAME , O_EXCL|O_RDWR|O_CREAT,\
			mqMode, &attrs);
	if (msgLog == -1) {
		logErr(ERROR, "mq_open", errno);
		return -1;
	}

	//Create semaphore for the engine :
  gsem_init(&sem, 0);
	//Create semaphore for the sensors acces :
	gsem_init(&sensorsSem,1); 
  gsem_give(&sensorsSem);
	
	//Init the device's structure content
	initMemory();
	
	//create various threads :
	//Start the thread with the defaults arguments, using the startSensorServer
	//function as entry point, with no arguments to this function.
  if (gthread_create(&sst,STACKSZ,startSensorServer,NULL)!=0){
		logErr(ERROR, "pthread_create failed for sensorServer thread", errno);
		handler(0);
	}
  if (gthread_create(&rrt,STACKSZ,startRestRcv,NULL)!=0){
		logErr(ERROR, "pthread_create failed for rest receive thread", errno);
		handler(0);
	}
  if (gthread_create(&dst,STACKSZ,startDispatchServer,&dispatchReq)!=0){
		logErr(ERROR, "pthread_create failed for dispatch Server thread",
				errno);
		handler(0);
	}
  if (gthread_create(&iet,STACKSZ,startEngine,NULL)!=0){
		logErr(ERROR, "pthread_create failes for inference engine thread",errno);
    handler(0);
	}

	//TODO : wait for both servers to have a client.

	//Wait for messages to log :
	received.mtype = INFO;
	received.mtext.level = DEBUG;
	memcpy(received.mtext.data, "test message received", 22);
  if (gmq_send(msgLog, (void *)&received, sizeof(struct mlog),0)==-1) {
		logErr(WARNING, "mq_send failed", errno);
	}
	for (stop = 0; stop != STOP;) {
    msgSize=gmq_receive(msgLog, (void*)&received, 210, NULL);
		if (msgSize == -1) {
			logErr(DEBUG, "mq_receive", errno);
			if (errno == EINTR) {
				continue; //we were interupted because a signal was caught
			} else {
				stop = STOP;
				continue;
			}
		}
		switch (received.mtype) {
		case STOP:
			stop = STOP;
			break;
		case ERR:
			logErr(received.mtext.level, received.mtext.data,
					received.mtext.errnb);
			break;
		case INFO:
			logMsg(received.mtext.level, received.mtext.data);
			break;
		default:
			logMsg(WARNING, "Received unexpected message");
		}
	}
	//Exit
	exit(0);
}

