
#ifndef SRC_THREAD_POOL_H_
#define SRC_THREAD_POOL_H_

#include "FreeRTOS.h"

typedef struct thread_pool_task
{
    void* params;
	void (*task)(void*);
} ThreadPoolTask_t;

BaseType_t initThreadPool();

BaseType_t submitTask(ThreadPoolTask_t* task);

#endif
