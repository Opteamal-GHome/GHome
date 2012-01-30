#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <jansson.h>
#include "mere.h"
#include "tcpserver.h"
#include "gestion_regles.h"

#define listen_port 4433
#define MAX_MSG_LENGTH 500

enum REQUEST_TYPE getRequestType(const char * type);
int requestTreatment(char *requestRule);

void * startRestRcv(void * args) {
  sigset_t set;
  int ret=0;
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

  socketRestClient = 0;
  sendLog(DEBUG, "rest reception thread started");
  socketServer = initServer(listen_port);
  if (socketServer == -1) {
    return NULL;
  }
  sendLog(DEBUG, "restServer initialized, socket descriptor : %d",
      socketServer);

  //INIT Memory for test
  initTestMemory();
  initMainRules(NULL); //TO CHANGE

  for (;;) {
    socketRestClient = waitClient(socketServer);
    if (socketRestClient == -1) {
      sendErr(WARNING, "restServer, wait failed", errno);
      return NULL;
    }
    sendLog(LOG, "restServer connected");

    for (ret=0; ret!=-1;) {
      ret=receive(socketRestClient, (void*) &sizeRequest, sizeof(long long));
      sendLog(DEBUG, "restServer resquest size: %lld", sizeRequest);

      if (sizeRequest < MAX_MSG_LENGTH && sizeRequest > 0) {
        //Message correct
        received = malloc(sizeRequest + 1);
        received[sizeRequest + 1] = '\0';
        receive(socketRestClient, (char*) received, sizeRequest); //sizeRequest

        if (requestTreatment(received) == TRUE) {
          sendLog(DEBUG, "Request done");
        } else {
          sendLog(DEBUG, "Request ignored");
        }
        free(received);
      } else {
        sendLog(DEBUG, "restServer resquest too large!!!");
      }
    }
    sendLog(LOG,"restServer deconected");
  }
  return NULL;
}

int requestTreatment(char *requestRule) {
  int priority;
  json_t * rule;
  json_t * requestJson = convertToJson(requestRule);
  if ((int) requestJson != FALSE) {
    const char * type = json_string_value(
        json_object_get(requestJson, "msgType"));
    if (type != NULL) {

      switch (getRequestType(type)) {
      case NEW_RULE:
        sendLog(DEBUG, "New Rule Request");
        priority = atoi(
            json_string_value(
                json_object_get(requestJson, "priority")));
          rule = json_object_get(requestJson, "rule");
          if (rule != NULL) {
            int bool = addRule(rule, priority);
            if (bool == TRUE) {
              sendLog(DEBUG, "Rule added");
              json_t * errorMsg= json_pack("{s:s, s:s}","msgType", "R_newRule","status", "ACCEPTED");
              char * msg = json_dumps(errorMsg, 0);
              sendNetMsg(REST,strlen(msg),msg);
              free(msg);
              saveRules("afterRequestTreatment.json");
              return TRUE;
            } else {
              sendLog(DEBUG, "Rule not added (not coherent)");
            }
          } else {
            sendLog(DEBUG, "Json request (NEW_RULE): rule missing");
          }
        break;
      default:
        break;

      }
    } else {
      sendLog(DEBUG, "Json request: field msgType missing");
    }
  } else {
    sendLog(DEBUG, "Json request not formated correctly");
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

