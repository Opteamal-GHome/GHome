/*
 * gestion_regles.c
 *
 *  Created on: 23 janv. 2012
 *      Author: mica
 */
#include "gestion_regles.h"

enum OPERATION_TYPE getOperator(const char * balise);

int actuatorAlreadyUpdated[NB_SENSORS];
int lastIndexUpdated;

json_t * getRuleByName(const char * name);
int removeRule(json_t *ruleToRemove);
int checkRule(json_t * rule);
int checkCondition(json_t * condition);
int updateActuator(int id);
int doActions(json_t * action);

void initMainRules(json_t * initSource) {

	root = json_array();

	if (!json_is_null(initSource)) {
		addRules(initSource, 0);
	}
}

void addRules(json_t * rules, int positionDepart) {
	int i;
	for (i = 0; i < json_array_size(rules); i++) {
		json_t *rule = json_array_get(rules, i);
		addRule(rule, positionDepart + i);
	}
}
/**
 * Insert la regle rule a la position "position" ou à la fin si la position est superieur au nb de regle
 */
int addRule(json_t * rule, int position) {
	int added = checkRuleCoherence(rule);
	if (added == TRUE
			&& (int) getRuleByName(
					json_string_value(
							json_object_get(rule, "ruleName"))) == FALSE) {
		if (json_array_insert(root, position, rule) == FALSE) {
			json_array_append_new(root, rule);
		} else {
			json_array_clear(rule);
		}
	}

	return added;
}

json_t * convertToJson(char * string) {
	json_t * jsonString;
	json_error_t error;
	jsonString = json_loads(string, 0, &error);

	if (!jsonString) {
		fprintf(stderr, "error: on line %d: %s\nBonus position:%d\nSource:%s",
				error.line, error.text, error.position, error.source);
		return (json_t *)FALSE;
	}
	return jsonString;
}

int removeRuleByName(char * name) {
	json_t *rule;

	rule = getRuleByName(name);
	if (!json_is_null(rule)) {
		return removeRule(rule);
	}
	return TRUE;
}

int removeRuleByIndex(size_t index) {
	return json_array_remove(root, (size_t) index);
}

int removeRule(json_t *ruleToRemove) {
	int i;
	for (i = 0; i < json_array_size(root); i++) {
		json_t *rule = json_array_get(root, i);
		if (rule == ruleToRemove) {
			return removeRuleByIndex(i);
		}
	}
	return TRUE;
}

int checkMainRulesCoherence() {
	int i;
	for (i = 0; i < json_array_size(root); i++) {
		json_t *rule = json_array_get(root, i);

		if (checkRuleCoherence(rule) == FALSE) {
			printf("Rule: %s removed\n",
					json_string_value(json_object_get(rule, "ruleName")));
			removeRuleByIndex(i);
		}
	}
	return TRUE;
}

json_t * getRuleByName(const char * name) {
	int i;
	const char * currentRuleName;
	if (name != NULL) {
		for (i = 0; i < json_array_size(root); i++) {
			json_t *rule = json_array_get(root, i);
			currentRuleName = json_string_value(
					json_object_get(rule, "ruleName"));

			if (strcmp(name, currentRuleName) == 0) {
				return rule;
			}
		}
	}
	return (json_t *) FALSE;
}

int checkRuleCoherence(json_t * rule) {

	json_t * conditions = json_object_get(rule, "conditions");
	if (conditions != NULL) {
		int i = 0;
		for (i = 0; i < json_array_size(conditions); i++) {
			json_t *condition = json_array_get(conditions, i);
			json_t *conditionType = json_object_get(condition, "type");

			if (!json_is_null(conditionType)) {
				const char * operande[2];
				const char * date;
				const char * type = json_string_value(conditionType);

				switch (getOperator(type)) {
				case SUP:
				case INF:
				case EQU:
					operande[0] = json_string_value(
							json_object_get(condition, "leftOp"));
					operande[1] = json_string_value(
							json_object_get(condition, "rightOp"));
					int i = 0;

					if (operande[0] != NULL && operande[1] != NULL) {
						//Si l'objet a 2 operandes

						for (; i < 2; i++) {
							if (memcmp("@", operande[i], 1) == 0) {
								//Si l'adressage est indirect

								char idCaptor[ID_SENSORS_SIZE];
								strcpy(idCaptor, (char *) operande[i] + 1);
								//On cherche si le capteur existe en memoire
								if (getMemDevice(atoi(idCaptor))
										== (struct DEVICE *) -1) {
									//sendLog(WARNING, "Rule Incoherence: sensor:%s inexistant",idCaptor);
									printf(
											"Rule Incoherence: sensor:%s inexistant\n",
											idCaptor);
									return FALSE;
								}
							}
						}

					} else {
						//Operande manquante
						//sendLog(WARNING, "Rule Incoherence: operande missing");
						printf("Rule Incoherence: operande missing\n");
						return FALSE;
					}

					break;
				case SUP_DATE:
				case INF_DATE:
				case EQU_DATE:
					date = json_string_value(
							json_object_get(condition, "date"));
					if (date == NULL) {
						//Operande manquante
						//sendLog(WARNING, "Rule Incoherence: date missing");
						printf("Rule Incoherence: date missing\n");
						return FALSE;
					}
					break;
				case UNKNOWN:
					return FALSE;
					break;
				default:
					break;
				}

			} else {
				//Aucune condition
				//sendLog(WARNING, "Rule Incoherence: condition missing");
				printf("Rule Incoherence: condition missing\n");
				return FALSE;
			}

		}
	}
	json_t * actions = json_object_get(rule, "actions");
	if (actions != NULL) {
		int i = 0;
		for (i = 0; i < json_array_size(actions); i++) {
			json_t *action = json_array_get(actions, i);
			json_t *actuator = json_object_get(action, "actuator");
			if (actuator != NULL
					&& (int) getMemDevice(
							atoi(json_string_value(actuator))) != FALSE) {
			} else {
				printf("Rule Incoherence: actuator %d missing\n",
						atoi(json_string_value(actuator)));
				return FALSE;
			}
		}
	}else{
		//Pas d'actions!!!
	}

	return TRUE;
}

int saveRules(char * fileName) {
	FILE * pFile;
	pFile = fopen(fileName, "w");
	if (pFile == NULL) {
		return -1;
	}
	json_dumpf(root, pFile, JSON_INDENT(3) | JSON_PRESERVE_ORDER);
	fclose(pFile);
	return TRUE;
}

enum OPERATION_TYPE getOperator(const char * balise) {
	enum OPERATION_TYPE operator;
	if (strcmp(balise, "sup") == 0)
		operator = SUP;
	else if (strcmp(balise, "inf") == 0)
		operator = INF;
	else if (strcmp(balise, "equ") == 0)
		operator = EQU;
	else if (strcmp(balise, "equ_date") == 0)
		operator = EQU_DATE;
	else if (strcmp(balise, "sup_date") == 0)
		operator = SUP_DATE;
	else if (strcmp(balise, "inf_date") == 0)
		operator = INF_DATE;
	else
		operator = UNKNOWN;
	return operator;
}

int checkCondition(json_t * condition) {
	json_t *conditionType = json_object_get(condition, "type");
	const char * operande[2];
	const char * date;
	int values[2];
	const char * type = json_string_value(conditionType);

	switch (getOperator(type)) {
	case SUP:
	case INF:
	case EQU:
		operande[0] = json_string_value(json_object_get(condition, "leftOp"));
		operande[1] = json_string_value(json_object_get(condition, "rightOp"));
		int i = 0;

		for (; i < 2; i++) {
			if (memcmp("@", operande[i], 1) == 0) {
				//Si l'adressage est indirect
				char idCaptor[ID_SENSORS_SIZE];
				strcpy(idCaptor, (char *) operande[i] + 1);
				values[i] = getMemDevice(atoi(idCaptor))->value;
			} else {
				//Adressage directe
				values[i] = atoi(operande[i]);
			}
		}
		switch (getOperator(type)) {
		case SUP:
			if (values[0] > values[1]) {
				return TRUE;
			}
			break;
		case INF:
			if (values[0] < values[1]) {
				return TRUE;
			}
			break;
		case EQU:
			if (values[0] == values[1]) {
				return TRUE;
			}
			break;
		default:
			break;
		}

		break;
	case SUP_DATE:
	case INF_DATE:
	case EQU_DATE:
		date = json_string_value(json_object_get(condition, "date"));
		int heure;
		int minute;
		sscanf(date, "%d:%d", &heure, &minute);
		time_t t;
		struct tm *heureActuelle;
		time(&t);
		heureActuelle = localtime(&t);
		switch (getOperator(type)) {
		case INF_DATE:
			if ((heure > heureActuelle->tm_hour)
					|| ((heure == heureActuelle->tm_hour)
							&& (minute > heureActuelle->tm_min))) {
				return TRUE;
			}
			break;
		case SUP_DATE:
			if ((heure < heureActuelle->tm_hour)
					|| ((heure == heureActuelle->tm_hour)
							&& (minute < heureActuelle->tm_min))) {
				return TRUE;
			}
			break;
		case EQU_DATE:
			if ((heure == heureActuelle->tm_hour)
					&& (heure == heureActuelle->tm_hour)) {
				return TRUE;
			}
			break;
		default:
			break;

		}
		break;
	default:
		break;

	}
	return FALSE;

}

int doActions(json_t * action) {
	json_t * actuator = json_object_get(action, "actuator");
	json_t * value = json_object_get(action, "value");

	if (updateActuator(atoi(json_string_value(actuator))) == TRUE) {
		setValue(atoi(json_string_value(actuator)),
				(int) atoi(json_string_value(value)));
		return TRUE;
	}
	return FALSE;
}

int checkRule(json_t * rule) {
	json_t * conditions = json_object_get(rule, "conditions");
	int i = 0;

	for (i = 0; i < json_array_size(conditions); i++) {
		json_t *condition = json_array_get(conditions, i);
		if (checkCondition(condition) != TRUE) {
			return FALSE;
		}
	}
	int j = 0;
	json_t * actions = json_object_get(rule, "actions");
	for (j = 0; j < json_array_size(actions); j++) {
		json_t *action = json_array_get(actions, j);
		if (doActions(action) != FALSE) {
			//Si la demande de l'action a été effectuée
			printf("Regle mise à jour %s\n",
					json_string_value(json_object_get(rule, "ruleName")));
		}
	}
	return TRUE;
}
void checkRules() {
	int i;
	lastIndexUpdated = 0;
	for (i = 0; i < json_array_size(root); i++) {
		json_t *rule = json_array_get(root, i);
		checkRule(rule);
	}
}

int updateActuator(int id) {
	int i = 0;
	for (i = 0; i < lastIndexUpdated; i++) {
		if (actuatorAlreadyUpdated[i] == id) {
			//Actuator deja mis a jour
			return FALSE;
		}
	}
	actuatorAlreadyUpdated[lastIndexUpdated] = id;
	lastIndexUpdated++;
	return TRUE;
}


