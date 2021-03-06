/*
 * gestion_capteurs.h
 *
 *  Created on: 26 janv. 2012
 *      Author: mica
 */

#ifndef GESTION_CAPTEURS_H_
#define GESTION_CAPTEURS_H_

#include "config.h"
#include <semaphore.h>
#define ID_SENSORS_SIZE 8

#undef FALSE
#define FALSE 0

#undef TRUE
#define TRUE 1

struct DEVICE { //Structure memoire
	int id;
	int value;
	char type;
	char role; // actuator or sensor, (ce champ va suremment disparaitre)
	unsigned int timestamp;
};

struct DEVICE * sensors;
//Semaphore-based acces protection for sensors, we use macros to allow us to change
//easily to and from posix/ghtreads semaphores.
#define SENSORS_SAFE() sem_wait(&sensorsSem)
#define SENSORS_UNSAFE() sem_post(&sensorsSem)
sem_t sensorsSem;

struct DEVICE * getMemDevice(int id);
void initMemory();
void removeMemDevice(int);
int setValue(int id, int value);
struct DEVICE * getMemDeviceByIndex(int index);
void initTestMemory();
char getSensorRole(char type);

#endif /* GESTION_CAPTEURS_H_ */


