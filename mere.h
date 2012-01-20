#ifndef __MERE_H
#define __MERE_H

#define STOP 1
#define ERR 2
#define INFO 3

enum logLvl{
  LOG,
  WARNING,
  ERROR,
  DEBUG
};

void * startSensorServer(void *);

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
