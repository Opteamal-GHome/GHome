#ifndef __TCPSERVER_H
#define __TCPSERVER_H

int initServer(int socketNb);
int waitClient(int socketd);
void transmit(int socketd, char * buff, int size);
int receive(int socketd, void * buff, int sizeMax);

#endif
