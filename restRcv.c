#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <json/json.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "mere.h"
#include "gthread.h"
#include "tcpserver.h"
#include "gestion_regles.h"
#include "restRcv.h"

typedef struct json_object json_object;

enum REQUEST_TYPE getRequestType(const char * type);
int requestTreatment(char *requestRule);
void sendAllDevices();
int newRuleRequest(json_object * requestJson);
int addDeviceToMsg(struct DEVICE* device, json_object * rootMsg);
char * transformCharToString(char a);
void removeRuleRequest(json_object * requestJson);
//char * getNextJSONRequest();

void * startRestRcv(void * args) {
	sigset_t set;
	int socketServer = 0;
	char* JSONRequest;

	sigemptyset(&set);
	sigaddset(&set, SIGTERM);
	sigaddset(&set, SIGINT);
	pthread_sigmask(SIG_UNBLOCK, &set, NULL);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
	//Those calls allow to uninstall the termination handler in this thread,
	//it is however a rather inelegant way to do it,
	//there must be some other way to achieve this.

	socketRestClient = 0;
	sendLog(DEBUG, "restServer reception thread started");
	socketServer = initServer(listen_port);
	if (socketServer == -1) {
		return NULL;
	}
	sendLog(DEBUG, "restServer initialized, socket descriptor : %d",
			socketServer);

	//INIT Memory for test
	//initTestMemory();

	//json_object * jsonOb = json_object_from_file("initJSON.json");
	//initMainRules(jsonOb); 
	initMainRules(NULL); //TODO SET THE DEFAULT FILE

	for (; socketRestClient != -1;) { 
		int msgLength = 0;
		int msgLengthReceived = 0;
		int lengthSizeReceived = 0;
		sendLog(LOG, "restServer wait for client");
		socketRestClient = waitClient(socketServer);
		if (socketRestClient == -1) {
			sendErr(WARNING, "restServer, wait failed", errno);
			return NULL;
		}
		sendLog(LOG, "restServer connected");

		while (lengthSizeReceived != -1 && msgLengthReceived != -1) {

			//Get a JSON request
			lengthSizeReceived = receive(socketRestClient, &msgLength,
					sizeof(msgLength));
			msgLength = ntohl(msgLength);
			if (lengthSizeReceived == sizeof(msgLength)) {
				JSONRequest = malloc(sizeof(char) * (msgLength + 1));
				msgLengthReceived = receive(socketRestClient, JSONRequest,
						msgLength);
				JSONRequest[msgLength] = '\0';
				if (msgLengthReceived != 0) {
					// Treat the JSON request
					if (JSONRequest != (char*) NULL
							&& msgLengthReceived == msgLength) {
						//sendLog(DEBUG, "Request : %s", JSONRequest);
						int success = requestTreatment(JSONRequest);

						if (success == TRUE) {
							sendLog(DEBUG, "restServer request done");
						} else {
							sendLog(DEBUG, "restServer request ignored");
						}

					}
				}
				free(JSONRequest);

			}
		}
		sendLog(LOG, "restServer deconected");
	}
	return NULL;
}

//Treat JSON request from REST
int requestTreatment(char *requestRule) {

	json_object * requestJson = convertToJson(requestRule);
	if ((int) requestJson != FALSE) {
		const char * type = json_object_get_string(
				json_object_object_get(requestJson, "msgType"));

		if (type != NULL) {

			switch (getRequestType(type)) {
			case NEW_RULE:
				sendLog(DEBUG, "restServer New Rule Request");
				if (newRuleRequest(requestJson) == TRUE) {
					gsem_give(&sem);
					return TRUE;
				}
				break;
			case GET_ALL_DEVICES:
				sendLog(DEBUG, "restServer Get All Device Request");
				sendAllDevices();
				json_object_put(requestJson);
				return TRUE;
				break;

			case GET_DEVICE:
				break;
			case REMOVE_RULE:
				sendLog(DEBUG, "restServer Remove Rule Request");
				removeRuleRequest(requestJson);
				gsem_give(&sem);
				json_object_put(requestJson);
				return TRUE;
				break;
			default:
				break;

			}
		} else {
			sendLog(DEBUG, "Json request: field msgType missing");
		}
		//If we come here, the requestJson is no longer needed
		json_object_put(requestJson);
	} else {
		json_object_put(requestJson);
		sendLog(DEBUG, "Json request not formated correctly");
	}
	return FALSE;
}
/*
 * Add the rule of requestJson if it can be added to the system rules and notify REST whether it's possible or not.
 */
int newRuleRequest(json_object * requestJson) {
	int priority;
	json_object * rule;

	priority = atoi(
			json_object_get_string(
					json_object_object_get(requestJson, "priority")));
	rule = json_object_object_get(requestJson, "rule");
	if (rule != NULL) {
		int bool = addRule(rule, priority);
		const char * ruleName = json_object_get_string(
				json_object_object_get(rule, "ruleName"));

		if (bool == TRUE) {
			sendLog(DEBUG, "Rule %s added", ruleName);
			json_object * errorMsg = json_object_new_object();

			json_object_object_add(errorMsg, "msgType",
					json_object_new_string("R_newRule"));
			json_object_object_add(errorMsg, "status",
					json_object_new_string("ACCEPTED"));

			char * msg = (char *) json_object_to_json_string(errorMsg);
			sendNetMsg(REST, strlen(msg), msg);
			json_object_put(errorMsg);

			return TRUE;
		} else {
			sendLog(DEBUG, "Rule %s not added (not coherent)", ruleName);
		}
	} else {
		sendLog(DEBUG, "Json request (NEW_RULE): rule missing");
	}

	return FALSE;
}

void removeRuleRequest(json_object * requestJson) {
	char * ruleName = (char *) json_object_get_string(
						json_object_object_get(requestJson, "ruleName"));
	removeRuleByName(ruleName);
	saveRules("RULES_STATUS.json");
}

enum REQUEST_TYPE getRequestType(const char * type) {
	enum REQUEST_TYPE request_Type;
	if (strcmp(type, "newRule") == 0) {
		request_Type = NEW_RULE;
	} else if (strcmp(type, "getAllDevices") == 0) {
		request_Type = GET_ALL_DEVICES;
	} else if (strcmp(type, "getDevice") == 0) {
		request_Type = GET_DEVICE;
	} else if (strcmp(type, "removeRule") == 0) {
		request_Type = REMOVE_RULE;
	} else{
		request_Type = UNKNOWN_REQUEST;
	}
	return request_Type;

}

void sendAllDevices() {
	int i = 0;
	struct DEVICE* device;
	json_object * rootMsg = json_object_new_object();
	json_object * sensorsArray = json_object_new_array();
	json_object * actuatorsArray = json_object_new_array();

	json_object_object_add(rootMsg, "msgType",
			json_object_new_string("R_getAllDevices"));
	json_object_object_add(rootMsg, "sensors", sensorsArray);
	json_object_object_add(rootMsg, "actuators", actuatorsArray);

	const char * str = json_object_to_json_string(rootMsg);
	if (str != NULL) {
	}

	for (; i < NB_SENSORS; i++) {
		//We get all devices
		for (;
				i < NB_SENSORS
						&& getMemDeviceByIndex(i) == (struct DEVICE*) NULL;
				i++) {
		}
		if (i < NB_SENSORS) {
			device = getMemDeviceByIndex(i);
			addDeviceToMsg(device, rootMsg);
		}
	}

	char * msg = (char *) json_object_to_json_string(rootMsg);
	sendNetMsg(REST, strlen(msg), msg);
	//sendLog(DEBUG, "restServer devices sent : %s", msg);
	json_object_put(rootMsg);

}

int addDeviceToMsg(struct DEVICE* device, json_object * rootMsg) {
	json_object * msg = json_object_new_object();
	//type == type (rest)

	if (device->role == 'S') {
		//Sensor
		json_object * sensorsArray = json_object_object_get(rootMsg, "sensors");
		json_object_array_add(sensorsArray, msg);

		char * type = transformCharToString(device->type);

		char idChar[6];
		sprintf(idChar, "%d", device->id);
		char deviceValue[6];
		sprintf(deviceValue, "%d", device->value);

		json_object_object_add(msg, "id", json_object_new_string(idChar));
		json_object_object_add(msg, "type", json_object_new_string(type));
		json_object_object_add(msg, "data",
				json_object_new_string(deviceValue));

	} else if (device->role == 'A') {
		//Actuator
		char idChar[6];
		sprintf(idChar, "%d", device->id);
		char deviceValue[6];
		sprintf(deviceValue, "%d", device->value);
		char * type = transformCharToString(device->type);

		json_object_object_add(msg, "id", json_object_new_string(idChar));
		json_object_object_add(msg, "type", json_object_new_string(type));
		json_object_object_add(msg, "data",
				json_object_new_string(deviceValue));

		json_object * actuatorArray = json_object_object_get(rootMsg,
				"actuators");

		json_object_array_add(actuatorArray, msg);

	}
	return TRUE;
}

char * transformCharToString(char a) {
	static char string[2];
	sprintf(string, "%c", a);
	return string;
}

/*
 char * getNextJSONRequest() {
 int bracket = 0;

 if (indiceStart >= msgReadLength) {
 msgReadLength = recv(socketRestClient, (char*) received, MAX_MSG_LENGTH,
 MSG_WAITFORONE); //
 //msgReadLength = recv(socketRestClient, (char*) received, 290,
 //				MSG_WAITALL);
 indiceStart = 0;
 indiceEnd = 0;
 sendLog(DEBUG, "restServer request length: %d", msgReadLength);
 }

 if (msgReadLength != -1) {

 //get the first {
 for (indiceStart = 0;
 received[indiceStart] != '{' && indiceStart < msgReadLength;
 indiceStart++) {
 }

 if (indiceStart < msgReadLength) {
 //Si on a {
 int done = FALSE;
 bracket++;

 for (indiceEnd = indiceStart + 1;
 done == FALSE && indiceEnd < msgReadLength; indiceEnd++) {
 if (received[indiceEnd] == '{') {
 bracket++;
 } else if (received[indiceEnd] == '}') {
 bracket--;
 }

 if (bracket == 0) {
 done = TRUE;
 sendLog(DEBUG, "restServer request received");
 }
 }

 if (done == TRUE) {
 //Si une requete JSON a ete detectee

 char * newRequest = malloc(
 sizeof(char) * (indiceEnd - indiceStart + 1));
 memset(newRequest, '\0',
 sizeof(char) * (indiceEnd - indiceStart + 1));
 newRequest[indiceEnd - indiceStart] = '\0';

 //On copie la requete
 memcpy(newRequest, (char*) received + indiceStart,
 indiceEnd - indiceStart);
 indiceStart = indiceEnd;

 return newRequest;
 } else {
 sendLog(DEBUG, "restServer request cut");
 //Si il n'y a pas eu suffisamment de }
 msgReadLength = receive(socketRestClient, (char*) received,
 MAX_MSG_LENGTH);
 if (msgReadLength != -1) {
 indiceStart = 0;
 indiceEnd = 0;
 int done = FALSE;

 for (indiceEnd = indiceStart + 1;
 done == FALSE && indiceEnd < msgReadLength;
 indiceEnd++) {

 if (received[indiceEnd] == '{') {
 bracket++;
 } else if (received[indiceEnd] == '}') {
 bracket--;
 }

 if (bracket == 0) {
 done = TRUE;
 sendLog(DEBUG, "restServer request received2");
 }
 indiceEnd++;
 }

 //					if (done == TRUE) {
 //						//Si une requete JSON a ete detectee
 //						char * newRequest = malloc(
 //								sizeof(indiceEnd - indiceStart + 1));
 //						newRequest[indiceEnd - indiceStart] = '\0';
 //
 //						//On copie la requete
 //						strncpy(newRequest, (char*) indiceStart,
 //								indiceEnd - indiceStart);
 //						indiceStart = indiceEnd;
 //						return newRequest;
 //					} else {
 //						//Rest fais n'importe quoi
 //						sendLog(DEBUG, "restServer request explosion!!!");
 //						indiceStart = 0;
 //						indiceEnd = 0;
 //					}

 }
 }

 }
 }
 return (char*) FALSE;
 }
 */


