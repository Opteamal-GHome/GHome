/*
 * gestion_regles.c
 *
 *  Created on: 23 janv. 2012
 *      Author: mica
 */
#include <stdio.h>
#include <stdlib.h>
#include "gestion_regles.h"
typedef struct json_object json_object;

enum OPERATION_TYPE getOperator(const char * balise);

int actuatorAlreadyUpdated[NB_SENSORS];
int lastIndexUpdated;

json_object * getRuleByName(const char * name);
int removeRule(json_object *ruleToRemove);
int checkRule(json_object * rule);
int checkCondition(json_object * condition);
int updateActuator(int id);
int doAction(json_object * action);

void initMainRules(json_object * initSource) {

	root = json_object_new_array();

	if (initSource != NULL) {
		addRules(initSource, 0);
	}
}

void addRules(json_object * rules, int positionDepart) {
	int i;
	for (i = 0; i < json_object_array_length(rules); i++) {
		json_object *rule = json_object_array_get_idx(rules, i);
		addRule(rule, positionDepart + i);
	}
}
/**
 * Insert la regle rule a la position "position" ou à la fin si la position est superieur au nb de regle (dont free rule succed
 */
int addRule(json_object * rule, int position) { //TODO use position
	int added = checkRuleCoherence(rule);
	if (added != TRUE) {
		return FALSE;
	}
	const char * ruleName = json_object_get_string(
						json_object_object_get(rule, "ruleName"));
	if ((int) getRuleByName(
			ruleName) == FALSE) {
		json_object_array_add(root, rule);
	} else {
		sendLog(DEBUG, "Rule Incoherence: name already in use %s",
				ruleName);
		json_object * errorMsg = json_object_new_object();
		json_object_object_add(errorMsg, "msgType",
				json_object_new_string("R_newRule"));
		json_object_object_add(errorMsg, "status",
				json_object_new_string("REFUSED"));
		json_object_object_add(errorMsg, "error",
				json_object_new_string("name_already_in_use"));

		char * msg = (char *) json_object_to_json_string(errorMsg);
		sendNetMsg(REST, strlen(msg), msg);
		//free(msg);
		return FALSE;
	}

	saveRules("RULES_STATUS.json");
	return TRUE;
}

json_object * convertToJson(char * string) {
	json_object * jsonString;
	jsonString = json_tokener_parse(string);

	if (!jsonString) {
		sendLog(DEBUG, "Invalid JSON msg");
		return (json_object *) FALSE;
	}
	return jsonString;
}

int removeRuleByName(char * name) {
	json_object *rule;

	rule = getRuleByName(name);
	if (rule != NULL) {
		return removeRule(rule);
	}
	return TRUE;
}

int removeRuleByIndex(size_t index) {
//TODO Tester
	json_object_put((json_object_array_get_idx(root, (size_t) index)));

	return TRUE;
}

int removeRule(json_object *ruleToRemove) {
	int i;
	for (i = 0; i < json_object_array_length(root); i++) {
		json_object *rule = json_object_array_get_idx(root, i);
		if (rule == ruleToRemove) {
			return removeRuleByIndex(i);
		}
	}
	return TRUE;
}

int checkMainRulesCoherence() {
	int i;
	for (i = 0; i < json_object_array_length(root); i++) {
		json_object *rule = json_object_array_get_idx(root, i);

		if (checkRuleCoherence(rule) == FALSE) {
			sendLog(DEBUG, "Rule: %s removed",
					json_object_get_string(json_object_object_get(rule, "ruleName")));
			removeRuleByIndex(i);
		}
	}
	return TRUE;
}

json_object * getRuleByName(const char * name) {
	int i;
	const char * currentRuleName;
	if (name != NULL) {
		for (i = 0; i < json_object_array_length(root); i++) {
			json_object *rule = json_object_array_get_idx(root, i);
			currentRuleName = json_object_get_string(json_object_object_get(rule, "ruleName"));

			if (strcmp(name, currentRuleName) == 0) {
				return rule;
			}
		}
	}
	return (json_object *) FALSE;
}

int checkRuleCoherence(json_object * rule) {

	json_object * conditions = json_object_object_get(rule, "conditions");
	if (conditions != NULL) {
		int i = 0;
		for (i = 0; i < json_object_array_length(conditions); i++) {
			json_object *condition = json_object_array_get_idx(conditions, i);
			json_object *conditionType = json_object_object_get(condition,
					"type");

			if (conditionType != NULL) {
				const char * operande[2];
				const char * date;
				const char * type = json_object_get_string(conditionType);

				switch (getOperator(type)) {
				case SUP:
				case INF:
				case EQU:
					operande[0] = json_object_get_string(json_object_object_get(condition, "leftOp"));
					operande[1] = json_object_get_string(json_object_object_get(condition, "rightOp"));
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
									sendLog(
											DEBUG,
											"Rule Incoherence: sensor:%s inexistant",
											idCaptor);
									json_object * errorMsg =
											json_object_new_object();

									json_object_object_add(errorMsg, "msgType",
											json_object_new_string("R_newRule"));
									json_object_object_add(errorMsg, "status",
											json_object_new_string("REFUSED"));
									json_object_object_add(errorMsg, "error",
											json_object_new_string("ID"));
									json_object_object_add(errorMsg, "cause",
											json_object_new_string(idCaptor));

									char * msg = (char *) json_object_to_json_string(errorMsg);
									sendNetMsg(REST, strlen(msg), msg);
									//free(msg);
									json_object_put(errorMsg);

									return FALSE;
								}
							}
						}

					} else {
						//Operande manquante
						sendLog(DEBUG, "Rule Incoherence: operande missing");
						json_object * errorMsg = json_object_new_object();

						json_object_object_add(errorMsg, "msgType",
								json_object_new_string("R_newRule"));
						json_object_object_add(errorMsg, "status", json_object_new_string("REFUSED"));
						json_object_object_add(errorMsg, "error",
								json_object_new_string("operande_missing"));

						char * msg = (char *) json_object_to_json_string(errorMsg);
						sendNetMsg(REST, strlen(msg), msg);
						//free(msg);
						json_object_put(errorMsg);
						return FALSE;
					}

					break;
				case SUP_DATE:
				case INF_DATE:
				case EQU_DATE:
					date = json_object_get_string(json_object_object_get(condition, "date"));
					if (date == NULL) {
						//Operande manquante
						sendLog(DEBUG, "Rule Incoherence: date missing");
						json_object * errorMsg = json_object_new_object();

						json_object_object_add(errorMsg, "msgType",
								json_object_new_string("R_newRule"));
						json_object_object_add(errorMsg, "status", json_object_new_string("REFUSED"));
						json_object_object_add(errorMsg, "error",
								json_object_new_string("date_missing"));

						char * msg =  (char *) json_object_to_json_string(errorMsg);
						sendNetMsg(REST, strlen(msg), msg);
						//free(msg);
						json_object_put(errorMsg);
						return FALSE;
					}
					break;
				case UNKNOWN:
					sendLog(DEBUG, "Rule Incoherence: operation type unknown");

					json_object * errorMsg = json_object_new_object();
					json_object_object_add(errorMsg, "msgType", json_object_new_string("R_newRule"));
					json_object_object_add(errorMsg, "status", json_object_new_string("REFUSED"));
					json_object_object_add(errorMsg, "error",
							json_object_new_string("operation_type_unknown"));

					char * msg =  (char *) json_object_to_json_string(errorMsg);
					sendNetMsg(REST, strlen(msg), msg);
					//free(msg);
					json_object_put(errorMsg);
					return FALSE;
					break;
				default:
					break;
				}

			} else {
				//Aucune condition
				sendLog(DEBUG, "Rule Incoherence: condition missing");

				json_object * errorMsg = json_object_new_object();
				json_object_object_add(errorMsg, "msgType", json_object_new_string("R_newRule"));
				json_object_object_add(errorMsg, "status", json_object_new_string("REFUSED"));
				json_object_object_add(errorMsg, "error", json_object_new_string("condition_missing"));

				char * msg =  (char *) json_object_to_json_string(errorMsg);
				sendNetMsg(REST, strlen(msg), msg);
				//free(msg);
				json_object_put(errorMsg);

				return FALSE;
			}

		}
	}
	json_object * actions = json_object_object_get(rule, "actions");
	if (actions != NULL) {
		int i = 0;
		for (i = 0; i < json_object_array_length(actions); i++) {
			json_object *action = json_object_array_get_idx(actions, i);
			json_object *actuator = json_object_object_get(action, "actuator");
			if (actuator != NULL
					&& (int) getMemDevice(atoi(json_object_get_string(actuator))) != FALSE) {
			} else {
				sendLog(DEBUG, "Rule Incoherence: actuator %d missing",
						atoi(json_object_get_string(actuator)));

				json_object * errorMsg = json_object_new_object();
				json_object_object_add(errorMsg, "msgType", json_object_new_string("R_newRule"));
				json_object_object_add(errorMsg, "status", json_object_new_string("REFUSED"));
				json_object_object_add(errorMsg, "error", json_object_new_string("ID"));
				json_object_object_add(errorMsg, "cause", json_object_new_string(json_object_get_string(actuator)));

				char * msg =  (char *) json_object_to_json_string(errorMsg);
				sendNetMsg(REST, strlen(msg), msg);
				//free(msg);
				json_object_put(errorMsg);

				return FALSE;
			}
		}
	} else {
		sendLog(DEBUG, "Rule Incoherence: no actions");
		json_object * errorMsg = json_object_new_object();
		json_object_object_add(errorMsg, "msgType", json_object_new_string("R_newRule"));
		json_object_object_add(errorMsg, "status", json_object_new_string("REFUSED"));
		json_object_object_add(errorMsg, "error", json_object_new_string("no_actions"));

		char * msg =  (char *) json_object_to_json_string(errorMsg);
		sendNetMsg(REST, strlen(msg), msg);
		//free(msg);
		json_object_put(errorMsg);
		return FALSE;
	}

	return TRUE;
}

int saveRules(char * fileName) {
	/*FILE * pFile;
	pFile = fopen(fileName, "w");
	if (pFile == NULL) {
		return -1;
	}
	const char * rootStatus = json_object_to_json_string(root);
	fprintf(pFile, "%s\n", rootStatus);
	fclose(pFile);*/
	json_object_to_file(fileName,root);
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

int checkCondition(json_object * condition) {
	json_object *conditionType = json_object_object_get(condition, "type");
	const char * operande[2];
	const char * date;
	int values[2];
	const char * type = json_object_get_string(conditionType);

	switch (getOperator(type)) {
	case SUP:
	case INF:
	case EQU:
		operande[0] = json_object_get_string(json_object_object_get(condition, "leftOp"));
		operande[1] = json_object_get_string(json_object_object_get(condition, "rightOp"));
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
		date = json_object_get_string(json_object_object_get(condition, "date"));
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

int doAction(json_object * action) {
	json_object * actuator = json_object_object_get(action, "actuator");
	json_object * value = json_object_object_get(action, "value");
	const char * actuatorName = json_object_get_string(actuator);
	const char * actuatorValue = json_object_get_string(value);

	if (updateActuator(atoi(actuatorName)) == TRUE) {
		sendLog(DEBUG, "(TODO) Request to set actuator: %s to %s",actuatorName,actuatorValue);
		//TODO send msg to pilote
		return TRUE;
	}
	return FALSE;
}

int checkRule(json_object * rule) {
	json_object * conditions = json_object_object_get(rule, "conditions");
	int i = 0;

	for (i = 0; i < json_object_array_length(conditions); i++) {
		json_object *condition = json_object_array_get_idx(conditions, i);
		if (checkCondition(condition) != TRUE) {
			return FALSE;
		}
	}
	int j = 0;
	json_object * actions = json_object_object_get(rule, "actions");
	for (j = 0; j < json_object_array_length(actions); j++) {
		json_object *action = json_object_array_get_idx(actions, j);
		if (doAction(action) != FALSE) {
			//Si la demande de l'action a été effectuée
			sendLog(DEBUG, "Engine Rule: %s activated",
					json_object_get_string(json_object_object_get(rule, "ruleName")));
		}
	}
	return TRUE;
}
void checkRules() {
	int i;
	lastIndexUpdated = 0;
	for (i = 0; i < json_object_array_length(root); i++) {
		json_object *rule = json_object_array_get_idx(root, i);
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



