#ifndef __TCPSERVER_H
#define __TCPSERVER_H

int initServer();
int waitClient();
void transmit(int socket, char * buff, int size);
int receive(int socket);

#endif
