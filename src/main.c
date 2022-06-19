
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "city_services.h"


TaskHandle_t defaultTaskHandle;
TaskHandle_t dispatcherHandle;
QueueHandle_t dispatcherQueueHandle;

CityServiceAttr_t services[TOTAL_SERVICES];
extern const char* names[TOTAL_SERVICES];
extern const int teams[TOTAL_SERVICES];

void StartDefaultTask(void *argument);
void dispatch(void *argument);

void serviceTask(void *argument);


int main(void) {
	#if (configSUPPORT_DYNAMIC_ALLOCATION == 1) 
	{
		dispatcherQueueHandle = xQueueCreate(16, sizeof(Request_t));
		if (dispatcherQueueHandle == NULL) {
			printf("Failed to create queue for dispatcher\r\n");
			return -1;
		}
		if (pdPASS != xTaskCreate(StartDefaultTask, "defaultTask", 128 * 4, NULL, 1, &defaultTaskHandle)) {
			printf("Failed to create default\r\n");
			return -1;
		}
		if (pdPASS != xTaskCreate(dispatch, "dispatcher", 128 * 4, NULL, 1, &dispatcherHandle)){
			printf("Failed to create task for dispatcher\r\n");
			return -1;
		}

		srand(time(NULL));
		{
			BaseType_t result = pdPASS;
			// setup city Services: create lock, handler task, queue
			for (int i = 0; i < TOTAL_SERVICES; i++) {
				services[i].name = names[i];
				services[i].queue = xQueueCreate(16, sizeof(Request_t));
				if (services[i].queue == NULL) {
					printf("Failed to create queue for service %s\r\n", names[i]);
					return -1;
				}

				services[i].semaphore = xSemaphoreCreateCounting(MAX_TEAMS, 0);
				if (services[i].semaphore == NULL) {
					printf("Failed to create semaphore for service %s\r\n", names[i]);
					return -1;
				}

				if (pdPASS != xTaskCreate(serviceTask, services[i].name, 128 * 4, &services[i], 1, &services[i].task)) {
					printf("Failed to create task for service %s\r\n", names[i]);
					return -1;
				}
			}
		}

		if (pdPASS != initThreadPool(&services[0])) {
			return -1;
		}
		vTaskStartScheduler();
	}
	#endif
	return 0;
}

void StartDefaultTask(void *argument) {
	Request_t req;
	BaseType_t sendResult;
	int delay;
	for (;;) {
		// create parameters for service request
		req.service = rand() % TOTAL_SERVICES;
		req.groups = rand() % MAX_TEAMS + 1;

		// try to put request to queue
		sendResult = xQueueSendToBack(dispatcherQueueHandle, &req, 0);
		if (sendResult == pdPASS) {
			printf("Generated request for service %s, groups %d\r\n", services[req.service].name, req.groups);
		} else {
			printf("Failed to send request to dispatcher\r\n");
		}
		delay = rand() % 10 + 1; // not bigger then portMAX_DELAY
		vTaskDelay(pdMS_TO_TICKS(delay * 1000));
	}
	vTaskDelete(NULL);
}

// routes requests from dispatcher queue to queue of specific service
void dispatch(void *argument) {
	Request_t req;
	BaseType_t receiveResult = pdPASS;
	BaseType_t mutexResult = pdPASS;
	BaseType_t putResult = pdPASS;
	CityServiceAttr_t serviceData;
	for (;;) {
		// try to receive request from queue
		if (xQueueReceive(dispatcherQueueHandle, &req, portMAX_DELAY) == pdPASS) {
			serviceData = services[req.service];
			// put request to queue of specific service
			if (xQueueSendToBack(serviceData.queue, &req, 0) != pdPASS) {
				printf("Request dropped. Service %s\r\n", services[req.service].name);
			}
		} else {
			printf("Dispatcher: receiving timeout\r\n");
		}
	}
	vTaskDelete(NULL);
}

// task for service
void serviceTask(void *argument) {
	CityServiceAttr_t* settings = (CityServiceAttr_t*) argument;
	Request_t req;
	for (;;) {
		if (xQueueReceive(settings -> queue, &req, portMAX_DELAY) == pdPASS) {
			// printf("Service %s handles, group %d\r\n", services[req.service].name, req.groups);
			for (int i = 0; i < req.groups; i++) {
				xSemaphoreGive(settings -> semaphore);			
			}
		} else {
			printf("Service %s receiving timeout\r\n", services[req.service].name);
		}
	}
	vTaskDelete(NULL);
}
