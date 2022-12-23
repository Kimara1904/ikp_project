#include "Queue.h"

IdMessagePair Dequeue(RingBufferQueue* queue)
{
	if (queue->cnt == 0)
	{
		char mess[] = "Empty queue";
		IdMessagePair idm = { -1, mess};
		idm.clientId = -1;
		
		return idm;
	}
	int index = queue->head;
	queue->head = (queue->head + 1) % RING_SIZE;
	queue->cnt--;
	return queue->data[index];
}

bool Enqueue(RingBufferQueue* queue, IdMessagePair data)
{
	if (queue == NULL)
	{
		return false;
	}
	if (queue->cnt == RING_SIZE)
	{
		return false;
	}
	queue->data[queue->tail] = data;
	queue->tail = (queue->tail + 1) % RING_SIZE;
	queue->cnt++;
	return true;
}

float Capacity(RingBufferQueue* queue)
{
	if (queue == NULL)
	{
		return -1;
	}

	return (float)(queue->cnt) / RING_SIZE * 100;
}