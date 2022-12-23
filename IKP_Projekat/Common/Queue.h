#include "Structs.h"
#define RING_SIZE 128 //??
typedef struct queue
{
	unsigned int tail;
	unsigned int head;
	unsigned int cnt;
	IdMessagePair data[RING_SIZE];
}RingBufferQueue;

IdMessagePair Dequeue(RingBufferQueue* queue);
bool Enqueue(RingBufferQueue* queue, IdMessagePair data);
float Capacity(RingBufferQueue* queue);