/*
 * gestion_regles.h
 *
 *  Created on: 23 janv. 2012
 *      Author: mica
 */
#include <string.h>
#include <jansson.h>
#include <stdio.h>
#include <time.h>
#include "gestion_capteurs.h"
#include "mere.h"

#ifndef GESTION_REGLES_H_
#define GESTION_REGLES_H_


#define TRUE 0
#define FALSE -1
#define DEBUG


enum OPERATION_TYPE {
	UNKNOWN, SUP, INF, EQU, SUP_DATE, INF_DATE, EQU_DATE
};

json_t *root;
/*
 * Initialise les regles internes a partir du Json passé en parametre.
 * Si il est nul, aucune regle n'est ajouté
 */
void initMainRules(json_t *);

/*
 * Renvoie FALSE si un capteur inexistant est present dans la regle, TRUE Sinon.
 */
int checkRuleCoherence(json_t * rule);

/*
 * Supprime toutes les regles internes qui utilisent au moins un capteur inexistant.
 */
int checkMainRulesCoherence();

/*
 * Ajout la regle a position dans les regles internes si elle est valide (capteur existants et nom inutilisé)
 */
int addRule(json_t *regle, int position);


void addRules (json_t * rules,int positionDepart);
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
 * Test toutes les regle, si leur conditions sont validees alors les actions associées sont effectué
 * Aucun controle des regles n'est effectue dans cette fonction.
 * Un actionneur ne peut etre modifie qu'une seule fois
 */
void checkRules();



#endif /* GESTION_REGLES_H_ */
