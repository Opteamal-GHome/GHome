/*
 * gestion_capteurs.c
 *
 *  Created on: 26 janv. 2012
 *      Author: mica
 */
#include "gestion_capteurs.h"
#include <string.h>

/**
 * Renvoie la structure associée du capteur d'ID id ou -1 si il n'existe pas
 */
struct DEVICE * getMemDevice(int id){
	struct DEVICE* sensor = NULL;
	int i=0;
	//Retourne un pointeur sur le sensor
	for(; i<nb_sensors && (sensors[i].id != id) ; i++){ }
	if ( i<nb_sensors )
		 sensor = &sensors[i];
	return sensor;
}

struct DEVICE * getMemDeviceByIndex(int index){
	struct DEVICE* sensor = (struct DEVICE*) NULL;
	if(index < nb_sensors && index >= 0 && sensors[index].id != 0){
		sensor = &sensors[index];
	}
	return sensor;
}

void removeMemDevice(id){
	struct DEVICE* sensor;
	sensor = getMemDevice(id);
	if(sensor != (struct DEVICE*) FALSE){
		sensor->id = 0;
	}

}

int setValue(int id, int value){
	if(id < nb_sensors && sensors[id].id != 0){
		sensors[id].value = value;
	}else{
		return FALSE;
	}
	return TRUE;
}
void initMemory(){
	memset(sensors,0,nb_sensors*sizeof(struct DEVICE));
}

void initTestMemory(){
  memset(sensors,0,nb_sensors*sizeof(struct DEVICE));
	sensors[0].id = 10;
	sensors[0].role = 'S';
	sensors[0].type = 'T';
	sensors[0].value = 42;

	sensors[1].id = 11;
	sensors[1].role = 'S';
	sensors[1].type = 'T';
	sensors[1].value = 35;

	sensors[2].id = 12;
	sensors[2].role = 'S';
	sensors[2].type = 'H';
	sensors[2].value = 42;

	sensors[3].id = 1;
	sensors[3].role = 'A';
	sensors[3].type = 'I';
	sensors[3].value = 42;

	sensors[4].id = 21;
	sensors[4].role = 'A';
	sensors[4].type = 'I';
	sensors[4].value = 42;
}




