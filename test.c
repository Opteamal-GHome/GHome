#include "tcpserver.h"

int main(int argc, char ** argv){
  int socketClient=0;
  int socketServer=0;
  int ret=0;
  socketServer=initServer();  
  socketClient=waitClient(socketServer);
  for (; ret!=-1;){
  ret = receive(socketClient);
  }
  return 0;
}
