#include "mere.h"
#include "config.h"
#include "gestion_regles.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_SIZE 80
#define CONFIG_FILE "./ghome.conf"

//default values :
#define SENSORS_LISTEN_PORT 8080
#define REST_LISTEN_PORT 8081
#define REST_CONNECT_PORT 421
#ifndef NB_SENSORS
  #define NB_SENSORS 20
#endif

static int parse_line(FILE * conf_file){
  char line[80];
  char key[25];
  int value;
  if(fgets(line,80,conf_file)==NULL){
    //Probably EOF
    return -1;
  }
  if(sscanf(line,"%24s %u",key,&value)!=2){
    return -1; //ligne invalide
  }
  if (strncmp(key,"CONNECT_REST_PORT",24)==0){
    rest_connect_port=value;
    return 1;    
  }
  if (strncmp(key,"LISTEN_REST_PORT",24)==0){
    rest_listen_port=value;
    return 1;    
  }
  if (strncmp(key,"LISTEN_SENSORS_PORT",24)==0){
    sensors_listen_port=value;
    return 1;    
  }
  if (strncmp(key,"NB_SENSORS",24)==0){
    nb_sensors=value;
    return 1;    
  }
  return -1;
}
int load_config(){
  //load conf file
  int fin=0;
  FILE * conf;
  sensors_listen_port=SENSORS_LISTEN_PORT;
  rest_listen_port=REST_LISTEN_PORT;
  rest_connect_port=REST_CONNECT_PORT;
  nb_sensors=NB_SENSORS;
  conf=fopen(CONFIG_FILE,"r");
  if (conf==NULL){
    sendErr(WARNING,"config file fopen ",errno);
    //initialize sensors array :
    sensors=malloc(nb_sensors*sizeof(struct DEVICE));
    actuatorAlreadyUpdated=malloc(nb_sensors*sizeof(int));
    return -1;
  }
  for(fin=0; fin!=-1;){
    fin=parse_line(conf);
  }
  //initialize sensors array :
  sensors=malloc(nb_sensors*sizeof(struct DEVICE));
  actuatorAlreadyUpdated=malloc(nb_sensors*sizeof(int));
  fclose(conf);
  return 0;
}
