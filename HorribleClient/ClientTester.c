/*
 ============================================================================
 Name        : ClientTester.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define SERVEURNAME "134.214.166.120" //"127.0.0.1"//
#define SERVEURSock 80

int to_server_socket = -1;
//0
char * jsonExample3= "{ \"msgType\":\"checkRules\"}";
char * jsonExample322 =
		"{ \"msgType\":\"newRule\","
				"\"priority\":\"1\","
				"\"rule\":{ \
\"ruleName\":\"Rule 2\", \
\"conditions\": [ \
{ \
\"type\" : \"inf\", \
\"leftOp\": \"1\", \
\"rightOp\" : \"@10\" \
}],\n \
\"actions\" : [ \
{ \
\"actuator\" : \"10\", \
\"value\" : \"1\"  \
}\n \
] \
} "
				"}";
char * jsonExample378 = "{ \"msgType\":\"getAllDevices\"}";
char * jsonExample365 = "{ \"msgType\":\"getAllRules\"}";
char * jsonExample3877 = "{ \"msgType\":\"removeRule\", \"ruleName\":\"Rule 2\" }";

int main(void) {

	char *server_name = SERVEURNAME;
	struct sockaddr_in serverSockAddr;
	struct hostent *serverHostEnt;
	long hostAddr;
	long status;
	char buffer[701];

	bzero(&serverSockAddr, sizeof(serverSockAddr));
	hostAddr = inet_addr(server_name);
	if ((long) hostAddr != (long) -1)
		bcopy(&hostAddr, &serverSockAddr.sin_addr, sizeof(hostAddr));
	else {
		serverHostEnt = gethostbyname(server_name);
		if (serverHostEnt == NULL) {
			printf("Prblm gethost\n");
			exit(0);
		}
		bcopy(serverHostEnt->h_addr, &serverSockAddr.sin_addr,
				serverHostEnt->h_length);
	}
	serverSockAddr.sin_port = htons(SERVEURSock);
	serverSockAddr.sin_family = AF_INET;

	printf("creation socket client\n");
	/* creation de la socket */
	if ((to_server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Prblm creation socket client\n");
		exit(0);
	}
	printf("demande de connection\n");
	/* requete de connexion */
	if (connect(to_server_socket, (struct sockaddr *) &serverSockAddr,
			sizeof(serverSockAddr)) < 0) {
		perror("connect");
		printf("Prblm demande de connection\n");
		exit(0);
	}

	/* envoie de donne et reception */
	//int msgLength = strlen(jsonExample);
	//int msgLength2 = strlen(jsonExample2);
	int msgLength3 = strlen(jsonExample3);
	//char length [4];
	//sprintf(length, "%d",msgLength);
	//printf("Write msg lenght: %s\n",length);
	int i = 0;
	int a;
	//int msgLenghtIndian = htonl(msgLength);
	//int msgLenghtIndian2 = htonl(msgLength2);
	int msgLenghtIndian3 = htonl(msgLength3);
	for (; i < 1; i++) {
/*		printf("Nb write len:%d : %d\n",
				write(to_server_socket, (void*) &msgLenghtIndian, sizeof(msgLenghtIndian)),
				msgLength);
		printf("Nb write:%d\n", write(to_server_socket, jsonExample,msgLength));

		a = read(to_server_socket, buffer, 700);
				buffer[a] = '\0';
				printf("Lut nb %d:%s\n", a, buffer);

		printf("Nb write len:%d : %d\n",
				write(to_server_socket, (void*) &msgLenghtIndian2, sizeof(msgLenghtIndian2)),
				msgLength2);
		printf("Nb write:%d\n", write(to_server_socket, jsonExample2,msgLength2));

		//write(to_server_socket,(void*) &(msgLength), sizeof(msgLength));
		//printf("Nb write:%d\n",write(to_server_socket, jsonExample, msgLength));

		a = read(to_server_socket, buffer, 700);
		buffer[a] = '\0';
		printf("Lut nb %d:%s\n", a, buffer);
*/
		printf("Nb write len:%d : %d\n",
				write(to_server_socket, (void*) &msgLenghtIndian3, sizeof(msgLenghtIndian3)),
				msgLength3);
		printf("Nb write:%d\n", write(to_server_socket, jsonExample3,msgLength3));

		//write(to_server_socket,(void*) &(msgLength), sizeof(msgLength));
		//printf("Nb write:%d\n",write(to_server_socket, jsonExample, msgLength));

		a = read(to_server_socket, buffer, 1400);
		buffer[a] = '\0';
		printf("Lut nb %d:%s\n", a, buffer);

	}

	/* fermeture de la connection */
	shutdown(to_server_socket, 2);
	close(to_server_socket);
	return 0;
}

