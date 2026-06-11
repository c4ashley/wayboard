#define _QUEUE_C_
#ifndef _QUEUE_H_
#include "queue.h"
#endif

#ifndef _FUTEX_C_
#include "futex.c"
#endif

bool Queue_Pop(struct KeyQueue* queue, KeyIndex* indexOut, uint8_t* stateOut)
{
	fwait(&queue->Lock);
	if (queue->Back == queue->Front)
	{
		fpost(&queue->Lock);
		return false;
	}

	uint64_t value = queue->Data[queue->Front];
	indexOut->raw = value & 0xFFFFFFFFu;
	*stateOut = (value >> 32);
	queue->Front = (queue->Front + 1) & 63;
	fpost(&queue->Lock);
	return true;
};

bool Queue_Push(struct KeyQueue* queue, KeyIndex index, uint8_t state)
{
	fwait(&queue->Lock);
	short back = queue->Back;
	short backNext = (back + 1) & 63;

	if (backNext == queue->Front)
	{
		fpost(&queue->Lock);
		return false;
	}

	queue->Data[back] = index.raw | ((uint64_t)state << 32);
	queue->Back = backNext;
	fpost(&queue->Lock);
	return true;
};

void Queue_Clear(struct KeyQueue* queue)
{
	fwait(&queue->Lock);
	queue->Front = queue->Back = 0;
	fpost(&queue->Lock);
};

bool Queue_IsEmpty(struct KeyQueue* queue)
{
	fwait(&queue->Lock);
	bool result = queue->Front == queue->Back;
	fpost(&queue->Lock);
	return result;
};

int Queue_Count(struct KeyQueue* queue)
{
	fwait(&queue->Lock);
	int result = ((queue->Back - queue->Front) + 64) & 63;
	fpost(&queue->Lock);
	return result;
}
