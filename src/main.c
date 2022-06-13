
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "city_services.h"
#include "semphr.h"


TaskHandle_t defaultTaskHandle;
TaskHandle_t dispatcherHandle;
QueueHandle_t dispatcherQueueHandle;

service_attr_t services[TOTAL_SERVICES];

void StartDefaultTask(void *argument);
void dispatch(void *argument);

void serviceTask(void *argument);
void teamTask(void *argument);


int main(void) {
	dispatcherQueueHandle = xQueueCreate(16, sizeof(request_t));
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
			services[i].queue = xQueueCreate(16, sizeof(request_t));
			if (services[i].queue == NULL) {
				printf("Failed to create queue for service %s\r\n", names[i]);
				return -1;
			}

			services[i].mutex = xSemaphoreCreateMutex();
			if (services[i].mutex == NULL) {
				printf("Failed to create mutex for service %s\r\n", names[i]);
				return -1;
			}

			services[i].semaphore = xSemaphoreCreateCounting(teams[i], teams[i]);
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
	vTaskStartScheduler();
	return 0;
}

void StartDefaultTask(void *argument) {
	request_t req;
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
			// wake up thread
			vTaskResume(dispatcherHandle);
		} else {
			printf("Failed to send request to dispatcher\r\n");
		}
		delay = rand() % 10; // not bigger then portMAX_DELAY
		vTaskDelay(pdMS_TO_TICKS(delay * 1000));
	}
	vTaskDelete(NULL);
}

// routes requests from dispatcher queue to queue of specific service
void dispatch(void *argument) {
	request_t req;
	BaseType_t receiveResult = pdPASS;
	BaseType_t mutexResult = pdPASS;
	BaseType_t putResult = pdPASS;
	service_attr_t serviceData;
	for (;;) {
		// try to receive request from queue
		receiveResult = xQueueReceive(dispatcherQueueHandle, &req, 0);
		if (receiveResult == pdPASS) {
			serviceData = services[req.service];
			mutexResult = xSemaphoreTake(serviceData.mutex, 0);
			if (mutexResult == pdPASS) {
				// put request to queue of specific service
				if (xQueueSendToBack(serviceData.queue, &req, 0) == pdPASS) {
					printf("Request Dispatched to service %s\r\n", services[req.service].name);
				}
				xSemaphoreGive(serviceData.mutex);
				// wake up service thread
				vTaskResume(serviceData.task);
			} else {
				printf("Failed to acquire lock\r\n");
			}
		} else {
			printf("Failed to get message from queue\r\n");
		}
		// wait for new messages in queue
		vTaskSuspend(dispatcherHandle);
	}
	vTaskDelete(NULL);
}

// task for service
void serviceTask(void *argument) {
	service_attr_t* settings = (service_attr_t*) argument;
	struct request req;
	BaseType_t mutexResult;
	task_params_t* params;
	for (;;) {
		if (xSemaphoreTake(settings -> mutex, 0) == pdPASS) {
			mutexResult = xQueueReceive(settings -> queue, &req, 0);
			xSemaphoreGive(settings -> mutex);

			if (mutexResult == pdPASS) {
				printf("Service %s handles, group %d\r\n", services[req.service].name, req.groups);
				for (int i = 0; i < req.groups; i++) {
					params = calloc(1, sizeof(task_params_t));
					if (params != NULL) {
						params -> semaphore = settings -> semaphore;
						params -> name = settings -> name;

						if (xTaskCreate(teamTask, NULL, 128 * 4, params, 1, NULL) != pdPASS) {
							printf("Failed to launch teamTask\n\r");
							free(params);
						}
					} else {
						printf("Failed to allocate memory for team params\r\n");
					}					
				}
			} else {
				printf("Error receiving message\r\n");
			}
		} else {
			printf("Failed to acquire lock\r\n");
		}
		vTaskSuspend(settings -> task);
	}
	vTaskDelete(NULL);
}

void teamTask(void *argument) {
	task_params_t *params = (task_params_t*) argument;
	printf("Exec task for service %s\n\r", params -> name);
	for (;;) {
		if (xSemaphoreTake(params -> semaphore, 0) == pdPASS) {
			vTaskDelay(pdMS_TO_TICKS(rand() % 10));
			xSemaphoreGive(params -> semaphore);
			free(params);
			params = NULL;
			break;
		}
	}
	vTaskDelete(NULL);
}
