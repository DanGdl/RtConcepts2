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

typedef struct Request {
	int service;
	int groups;
} Request_t;

typedef struct CityServiceAttr {
	QueueHandle_t queue;
	SemaphoreHandle_t semaphore;
	const char* name;
	TaskHandle_t task;
} CityServiceAttr_t;


BaseType_t initThreadPool(CityServiceAttr_t* services);

#endif /* SRC_CITY_SERVICES_H_ */
