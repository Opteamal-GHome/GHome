#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <jansson.h>
#include "mere.h"
#include "tcpserver.h"
#include "gestion_regles.h"

#define listen_port 443
#define MAX_MSG_LENGTH 200
static int socketClient = 0;

enum REQUEST_TYPE getRequestType(const char * type);
int requestTreatment(char *requestRule);

void * startRestRcv(void * args) {
	sigset_t set;
	int socketServer = 0;
	long long sizeRequest;
	char *received; //thos has to be changed to something real^^

	sigemptyset(&set);
	sigaddset(&set, SIGTERM);
	sigaddset(&set, SIGINT);
	pthread_sigmask(SIG_UNBLOCK, &set, NULL);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
	//Those calls allow to uninstall the termination handler in this thread,
	//it is however a rather inelegant way to do it,
	//there must be some other way to achieve this.

	sendLog(DEBUG, "rest messages reception started");
	socketServer = initServer(listen_port);
	if (socketServer == -1) {
		return NULL;
	}
	sendLog(DEBUG, "restServer initialized, socket descriptor : %d",
			socketServer);
	socketClient = waitClient(socketServer);
	if (socketClient == -1) {
		sendErr(WARNING, "sensorServer, wait failed", errno);
		return NULL;
	}
	sendLog(DEBUG, "restServer client client connected");

	while (1) {
		receive(socketClient, (void*) &sizeRequest, 6);
		sendLog(DEBUG, "restServer resquest size: %lld", sizeRequest);

		if (sizeRequest < MAX_MSG_LENGTH) {
			//Message correct
			received = malloc(sizeRequest + 1);
			received[sizeRequest + 1] = '\0';
			receive(socketClient, (void*) &received, sizeRequest);

			if (requestTreatment(received) == TRUE) {
				sendLog(DEBUG, "Request done");
			} else {
				sendLog(DEBUG, "Request ignored");
			}

		} else {
			sendLog(DEBUG, "restServer resquest too large!!!");
		}
	}
	return NULL;
}

int requestTreatment(char *requestRule) {
	int priority;
	json_t * rule;
	json_t * requestJson = convertToJson(requestRule);
	if ((int)requestJson != FALSE){
		const char * type = json_string_value(
				json_object_get(requestJson, "msgType"));
		switch (getRequestType(type)) {
		case NEW_RULE:
			priority = atoi(
					json_string_value(json_object_get(requestJson, "priority")));
			rule = json_object_get(requestJson, "rule");
			int bool = addRule(rule, priority);
			if (bool == TRUE) {
				sendLog(DEBUG, "Rule added");
				saveRules("afterRequestTreatment.json");
				return TRUE;
			} else {
				sendLog(DEBUG, "Rule not added (not coherent)");
			}
			break;
		default:
			break;

		}
	}else{
		sendLog(DEBUG, "Request not formated correctly");
	}

	return FALSE;
}

enum REQUEST_TYPE getRequestType(const char * type) {
	enum REQUEST_TYPE request_Type;
	if (strcmp(type, "newRule") == 0)
		request_Type = NEW_RULE;
	else
		request_Type = UNKNOWN_REQUEST;
	return request_Type;

}


