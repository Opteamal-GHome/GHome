/*
 * gestion_regles.h
 *
 *  Created on: 23 janv. 2012
 *      Author: mica
 */
#ifndef GESTION_REGLES_H_
#define GESTION_REGLES_H_

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "gestion_capteurs.h"
#include "mere.h"
#include "json/json.h"


enum OPERATION_TYPE {
	UNKNOWN, SUP, INF, EQU, SUP_DATE, INF_DATE, EQU_DATE
};

enum REQUEST_TYPE {
	NEW_RULE,UNKNOWN_REQUEST, GET_ALL_DEVICES, GET_DEVICE, REMOVE_RULE, GET_ALL_RULES, CHECK_RULES, RESET_RULES, CHANGE_RULES_PRIORITIES
};

int * actuatorAlreadyUpdated; //allocated in config.c

typedef struct json_object json_object;
json_object *root;
/*
 * Initialise les regles internes a partir du Json passé en parametre.
 * Si il est nul, aucune regle n'est ajouté
 */
void initMainRules(json_object *);

/*
 * Renvoie FALSE si un capteur inexistant est present dans la regle, TRUE Sinon.
 */
int checkRuleCoherence(json_object * rule);

/*
 * Supprime toutes les regles internes qui utilisent au moins un capteur inexistant.
 */
int checkMainRulesCoherence();

/*
 * Ajout la regle a position dans les regles internes si elle est valide (capteur existants et nom inutilisé)
 */
int addRule(json_object *regle, int position);

/*
 * Ajoute si possible les regles (si valide)
 */
void addRules (json_object * rules,int positionDepart);
/*
 * Ajout les regles a partir de position dans les regles internes si elles sont valides (capteur existants et nom inutilisé)
 */
int saveRules(char *);

/*
 * Supprime des regles internes la regle ayant pour nom "name"
 */
int removeRuleByName(char * name);

/*
 * Supprime des regles internes la regle ayant pour indice index et décale toute l'indexation
 */
int removeRuleByIndex(size_t index);

/*
 * Test toutes les regles, si leur conditions sont validees alors les actions associées sont effectué
 * Aucun controle des regles n'est effectue dans cette fonction.
 * Un actionneur ne peut etre modifie qu'une seule fois par check
 */
void checkRules();

/*
 * Transforme la chaine de caractere passée en parametre et la convertie en json_object *
 */
json_object * convertToJson(char * string);

/*
 * Efface toutes les regles en mémoire
 */
void resetMainRules();

/*
* Change la priorite des regles
*/
void changeRulesPriorities(json_object * rulesArray);

#endif /* GESTION_REGLES_H_ */


