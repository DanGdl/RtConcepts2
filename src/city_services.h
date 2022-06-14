/*
 * city_services.h
 *
 *  Created on: Jul 10, 2021
 *      Author: max
 */

#ifndef SRC_CITY_SERVICES_H_
#define SRC_CITY_SERVICES_H_

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"


#define TOTAL_SERVICES 3

// available services
#define FIRE_TEAM 0
#define POLICE 1
#define AMBULANCE 2

// names for available services
#define NAME_FIRE_TEAM "FireTeam"
#define NAME_POLICE "Police"
#define NAME_AMBULANCE "Ambulance"

// amount of teams in every service
#define FIRE_TEAM_SIZE 2
#define POLICE_SIZE 3
#define AMBULANCE_SIZE 4

#define MAX_TEAMS 5

const char* names[TOTAL_SERVICES] = {NAME_FIRE_TEAM, NAME_POLICE, NAME_AMBULANCE};

const int teams[TOTAL_SERVICES] = {FIRE_TEAM_SIZE, POLICE_SIZE, AMBULANCE_SIZE};

typedef struct Request {
	int service;
	int groups;
} Request_t;

typedef struct CityServiceAttr {
	QueueHandle_t queue;
	SemaphoreHandle_t mutex;
	SemaphoreHandle_t semaphore;
	const char* name;
	TaskHandle_t task;
} CityServiceAttr_t;

#endif /* SRC_CITY_SERVICES_H_ */
