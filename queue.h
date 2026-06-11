#define _QUEUE_H_
#ifndef _COMMON_H_
#include "common.h"
#endif

#ifndef _FUTEX_H_
#include "futex.h"
#endif

struct KeyQueue
{
	uint64_t Data[64];
	short    Front;
	short    Back;
	futex_t  Lock;
};

bool Queue_Pop(struct KeyQueue* queue, KeyIndex* indexOut, uint8_t* stateOut);
bool Queue_Push(struct KeyQueue* queue, KeyIndex indexIn, uint8_t stateIn);
bool Queue_IsEmpty(struct KeyQueue* queue);
void Queue_Clear(struct KeyQueue* queue);
int  Queue_Count(struct KeyQueue* queue);
