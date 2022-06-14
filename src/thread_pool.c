
#include "thread_pool.h"

#include "task.h"
#include "queue.h"
#include "semphr.h"

#define POOL_SIZE 10
#define SIZE_POOL_QUEUE 20
#define SIZE_TASK_STACK 200

static StaticQueue_t poolQueue;
static uint8_t poolQueueBuffer[SIZE_POOL_QUEUE * sizeof(ThreadPoolTask_t)];
static QueueHandle_t queue;

static StaticSemaphore_t xMutexBuffer;
static SemaphoreHandle_t mutex;


static StackType_t poolTasksStacks[POOL_SIZE * SIZE_TASK_STACK];
static StaticTask_t poolTasksBuffers[POOL_SIZE];
static int poolTaskParams[POOL_SIZE] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
static const char* const poolTaskNames[POOL_SIZE] = {
    "poolTask0",
    "poolTask1",
    "poolTask2",
    "poolTask3",
    "poolTask4",
    "poolTask5",
    "poolTask6",
    "poolTask7",
    "poolTask8",
    "poolTask9"
};
static TaskHandle_t tasks[POOL_SIZE];

void poolTask(void* params) {
    int idx = *((int*) params);
    ThreadPoolTask_t taskInfo;
    for (;;) {
        if (pdPASS == xSemaphoreTake(mutex, 0)) {
            memset(&taskInfo, 0, sizeof(taskInfo));
            if (pdPASS == xQueueReceive(queue, &taskInfo, 0)) {
                (*taskInfo.task)(taskInfo.params);
            }
            xSemaphoreGive(mutex);
        }
        if (idx < POOL_SIZE) {
            vTaskSuspend(tasks[idx]);
        }
    }
}


BaseType_t initThreadPool() {
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
        queue = xQueueCreate(SIZE_POOL_QUEUE, sizeof(ThreadPoolTask_t));
        if (queue == NULL) {
            printf("Failed to create queue for thread pool\r\n");
            return pdFAIL;
        }
        mutex = xSemaphoreCreateMutex();
        if (mutex == NULL) {
            printf("Failed to create mutex for thread pool\r\n");
            return pdFAIL;
        }
        for (int i = 0; i < POOL_SIZE; i++) {
            if (pdPASS != xTaskCreate(poolTask, poolTaskNames[i], SIZE_TASK_STACK, (void*) &poolTaskParams[i], 1, &tasks[i])) {
                printf("Failed to create task %d for thread pool\r\n", i);
                return pdFAIL;
            }
        }
    }
    #endif
    return pdPASS;
}

static int counter = 0;

BaseType_t submitTask(ThreadPoolTask_t* task) {
    BaseType_t result = xSemaphoreTake(mutex, 0);
    if (pdPASS == result) {
        result = xQueueSendToBack(queue, task, 0);
        xSemaphoreGive(mutex);
        for (int i = counter; i < POOL_SIZE; i++) {
            TaskHandle_t handle = tasks[i];
            if (eSuspended == eTaskGetState(handle)) {
                vTaskResume(handle);
            }
        }
        counter++;
        counter = counter % POOL_SIZE;
    }
    return result;
}
