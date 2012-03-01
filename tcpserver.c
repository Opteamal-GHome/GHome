//Module containing basic operations allowing to use a tcp server
//Author(s) : Rapahel C.
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <gipcs.h>
#include <gthread.h>
#include "mere.h"

#define forward_address "127.0.0.1"
#define forward_port 1337

//Connect to the rest server.
int startUpdateSender(struct sockaddr_in client_address) {

	struct sockaddr_in server_address;

  memset(&server_address,0,sizeof(struct sockaddr_in)); //clear struct
	server_address.sin_family = AF_INET; //ipv4
  //Do not use the client_address when tunneling, hardcode the local address here instead !
	if (inet_pton(AF_INET, inet_ntoa(client_address.sin_addr)/*127.0.0.1*/,\
		&server_address.sin_addr) == -1) {
		sendErr(WARNING,"inet_pton ",errno);
		return -1;
	}
	server_address.sin_port = htons(rest_connect_port);
	server_address.sin_family = AF_INET;

	/* creation de la socket */
	if ((socketRestServer = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		sendLog(DEBUG, "update request: socket creation failed");
		return -1;
	}
  sendLog(DEBUG,"new connection to remote rest :"); 
  sendLog(DEBUG,"\tremote port : %d", ntohs(server_address.sin_port));
  sendLog(DEBUG,"\tremote address : %s",inet_ntoa(server_address.sin_addr));
	/* requete de connexion */
	if (connect(socketRestServer, (struct sockaddr *) &server_address,
			sizeof(server_address)) < 0) {
		sendErr(DEBUG, "update request: connection request failed",errno);
		return -1;
	}
	return 0;
}

//Init a server on a local socket :
int initServer(listen_port){

	int listen_socketd;
	uint16_t port;
	struct sockaddr_in own_address;
	int options = 1;

	port = htons(listen_port);
	listen_socketd = gsocket(AF_INET, SOCK_STREAM, 0);

	if(listen_socketd == -1) {
		sendErr(ERROR,"socket creation ",errno);
		return -1; //fail to create new socket
	}
	if (setsockopt(listen_socketd,SOL_SOCKET,SO_REUSEADDR,&options,4)) {
		sendErr(WARNING,"setsockopt ",errno);
	}
	memset(&own_address,0,sizeof(struct sockaddr_in)); //clear struct
	own_address.sin_family = AF_INET; //ipv4
	own_address.sin_port = port; //listening port
	own_address.sin_addr.s_addr = INADDR_ANY; //own address... 

	if (bind(listen_socketd, (struct sockaddr *) &own_address, sizeof(own_address)) < 0) {
		//fail to name socket;
		sendErr(ERROR,"bind ",errno);
		return -1;
	}

	if (listen(listen_socketd,1) < 0)
	{
		sendErr(ERROR,"listen ",errno);
		return -1;
	}
	sendLog(DEBUG,"Server listenning on port %d",ntohs(port));
	//printf("Server listenning on port %d\n",ntohs(port));
  return listen_socketd;
}

//Wait for someone to connect on a socket created with initServer()
int waitClient(int listenSocket, struct sockaddr_in * client){

	int request_socketd;
  struct sockaddr_in client_address;
	socklen_t client_size = sizeof(client_address);

  memset(&client_address,0,sizeof(struct sockaddr_in));
  request_socketd = gaccept(listenSocket,\
    (struct sockaddr *) &client_address, &client_size);
  if (request_socketd == -1)
  {
    sendErr(ERROR,"accept ",errno);
    return -1;	
  }
  sendLog(DEBUG,"new connection"); 
  sendLog(DEBUG,"\tremote port : %d", ntohs(client_address.sin_port));
  sendLog(DEBUG,"\tremote address : %s",inet_ntoa(client_address.sin_addr));

  if (client!=NULL){
    memcpy(client,&client_address,sizeof(struct sockaddr_in));
  }
	return request_socketd;
}

void closeClient(int clientSock){

  sendLog(DEBUG,"Client disconected");
  shutdown(clientSock, SHUT_RDWR);
	close(clientSock);
}

//receive exactly the requested number of bytes.
int receive(int socket, char * buff, int size){
  size_t ret = 0;
  size_t bytesRcv = 0;
  //int i=0;
  for (bytesRcv=0; bytesRcv<size; ){
    ret=grecv(socket,(void*)(buff+bytesRcv),size-ret,0);
    bytesRcv+=ret;
    if (ret==0){ //Normal deco
      closeClient(socket);
      return -1;
    }else if(ret==-1){ //Abnormal deco
      sendErr(DEBUG,"recv on socket failed",errno);
      return -1;
    }else{
      sendLog(DEBUG,"Received %d bytes.",ret);
      /*
      for (i=0; i<ret; i++){
        //sendLog(DEBUG,"%hhx ",buff[i]);
      }
      */
    }
  }
  return ret;
}

//transmit, useless wrapper to keep a coherence with the tcp-api
int transmit(int socket, char * buff, int size){
  if (gsend(socket, buff, size,0)==-1)
  {
    sendErr(DEBUG,"send on socket failed",errno); 
    return -1;
  }
  return 0;
}
