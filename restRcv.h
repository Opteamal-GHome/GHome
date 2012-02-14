/*
 * restRcv.h
 *
 *  Created on: 7 févr. 2012
 *      Author: mica
 */

#ifndef RESTRCV_H_
#define RESTRCV_H_

#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <json/json.h>
#include <sys/socket.h>
#include "mere.h"
#include "tcpserver.h"
#include "gestion_regles.h"

#define listen_port 8080
#define MAX_MSG_LENGTH 500

/*
 * Lance le traitement des echanges avec le client REST.
 */
//void * startRestRcv(void * args);
//void transmitUpdate(int id, int value);


#endif /* RESTRCV_H_ */


