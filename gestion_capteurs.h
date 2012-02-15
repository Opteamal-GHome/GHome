/*
 * gestion_capteurs.h
 *
 *  Created on: 26 janv. 2012
 *      Author: mica
 */

#ifndef GESTION_CAPTEURS_H_
#define GESTION_CAPTEURS_H_

#define NB_SENSORS 50
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

struct DEVICE sensors[NB_SENSORS];

struct DEVICE * getMemDevice(int id);
void initMemory();
void removeMemDevice(int);
int setValue(int id, int value);
struct DEVICE * getMemDeviceByIndex(int index);
void initTestMemory();

#endif /* GESTION_CAPTEURS_H_ */


