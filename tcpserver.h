#ifndef __TCPSERVER_H
#define __TCPSERVER_H
#include <netinet/in.h>
int initServer(int socketNb);
//Returns -1 in case of error
int waitClient(int socketd,struct sockaddr_in * client_address);
//Returns -1 in case of error
int transmit(int socketd, char * buff, int size);

int receive(int socketd, void * buff, int size);
//Returns sizeMax if the call succeded or -1 otherwise
//If the call succed it always puts 'bytesNb' bytes into 'buff'.

//init the socket for sending update
int startUpdateSender(struct sockaddr_in client);
#endif
