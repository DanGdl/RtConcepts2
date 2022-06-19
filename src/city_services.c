
#include "city_services.h"

#define POOL_SIZE 10
#define SIZE_POOL_QUEUE 20
#define SIZE_TASK_STACK 200

#if (configSUPPORT_STATIC_ALLOCATION == 1)
{
    static StackType_t poolTasksStacks[POOL_SIZE * SIZE_TASK_STACK];
    static StaticTask_t poolTasksBuffers[TOTAL_SERVICES * MAX_TEAMS];
}
#endif

static const char* const poolTaskNames[TOTAL_SERVICES * MAX_TEAMS] = {
    "poolTask0",
    "poolTask1",
    "poolTask2",
    "poolTask3",
    "poolTask4",
    "poolTask5",
    "poolTask6",
    "poolTask7",
    "poolTask8",
    "poolTask9",
    "poolTask10",
    "poolTask11",
    "poolTask12",
    "poolTask13",
    "poolTask14"
};
static TaskHandle_t tasks[TOTAL_SERVICES * MAX_TEAMS];

const char* names[TOTAL_SERVICES] = {NAME_FIRE_TEAM, NAME_POLICE, NAME_AMBULANCE};

const int teams[TOTAL_SERVICES] = {FIRE_TEAM_SIZE, POLICE_SIZE, AMBULANCE_SIZE};


void poolTask(void* argument);

BaseType_t initThreadPool(CityServiceAttr_t* services) {
    #if (configSUPPORT_STATIC_ALLOCATION == 1)
    {
        queue = xQueueCreateStatic(SIZE_POOL_QUEUE, sizeof(ThreadPoolTask_t), poolQueueBuffer, &poolQueue);
        if (queue == NULL) {
            printf("Failed to create queue for thread pool\r\n");
            return pdFAIL;
        }
        mutex = xSemaphoreCreateMutexStatic(&xMutexBuffer);
        if (mutex == NULL) {
            printf("Failed to create mutex for thread pool\r\n");
            return pdFAIL;
        }
        for (int i = 0; i < POOL_SIZE; i++) {
            tasks[i] = xTaskCreateStatic(poolTask, poolTaskNames[i], SIZE_TASK_STACK, (void*) poolTaskParams[i], 1, poolTasksStacks + SIZE_TASK_STACK * i, poolTasksBuffers + i);
            if (tasks[i] == NULL) {
                printf("Failed to create task %d for thread pool\r\n", i);
                return pdFAIL;
            }
        }
        
    }
    #endif
    #if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
    {
        for (int i = 0; i < TOTAL_SERVICES; i++) {
            for (int j = 0; j < teams[i]; j++) {
                if (pdPASS != xTaskCreate(
                    poolTask, poolTaskNames[i * MAX_TEAMS + j], SIZE_TASK_STACK,
                     (void*) (services + i), 1, &tasks[i * MAX_TEAMS + j]
                    )
                ) {
                    printf("Failed to create task %d for thread pool\r\n", i * MAX_TEAMS + j);
                    return pdFAIL;
                }
            }
        }
    }
    #endif
    return pdPASS;
}

void poolTask(void* argument) {
    const CityServiceAttr_t* const params = (CityServiceAttr_t*) argument;
	for (;;) {
		if (xSemaphoreTake(params -> semaphore, portMAX_DELAY) == pdPASS) {
			int delay = ((rand() % 50) + 10) * 10;
			printf("Exec task, service %s, delay %d, count %d, task %s\r\n", params -> name, delay, uxSemaphoreGetCount(params -> semaphore), pcTaskGetName(NULL));
			vTaskDelay(pdMS_TO_TICKS(delay));
		}
	}
    vTaskDelete(NULL);
}
