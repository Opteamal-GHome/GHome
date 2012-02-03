/*
 * gestion_capteurs.h
 *
 *  Created on: 26 janv. 2012
 *      Author: mica
 */

#ifndef GESTION_CAPTEURS_H_
#define GESTION_CAPTEURS_H_

#define NB_SENSORS 10
#define ID_SENSORS_SIZE 8
#define TRUE 0
#define FALSE -1

struct DEVICE{ //Structure memoire
	int id;
	int value;
	char type;
	unsigned long long timestamp;
};

struct DEVICE sensors[NB_SENSORS];//Pr modeliser la memoire

struct DEVICE * getMemDevice(int id);
void initTestMemory();
void removeMemDevice(int);
int setValue(int id, int value);

#endif /* GESTION_CAPTEURS_H_ */
