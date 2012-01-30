#ifndef __TCPSERVER_H
#define __TCPSERVER_H

int initServer(int socketNb);
//Returns -1 in case of error
int waitClient(int socketd);
//Returns -1 in case of error
int transmit(int socketd, char * buff, int size);

int receive(int socketd, void * buff, int size);
//Returns sizeMax if the call succeded or -1 otherwise
//If the call succed it always puts 'bytesNb' bytes into 'buff'.

#endif
