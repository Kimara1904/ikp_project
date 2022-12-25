#pragma once
#ifndef STRUCT_H
#define STRUCT_H
#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "conio.h"

#define DEFAULT_BUFLEN 512
#define RING_SIZE 128

typedef struct client_struct_list_item
{
	int id;
	SOCKET socket;
	char request_message[DEFAULT_BUFLEN];
}ClientListItem;

typedef struct worker_role_struct
{
	int id;
	SOCKET socket;
	char message_box[DEFAULT_BUFLEN];
	CRITICAL_SECTION cs;
	HANDLE semaphore;
}WorkerRole;

typedef struct clientid_message_pair
{
	int clientId;
	char message[DEFAULT_BUFLEN];
}IdMessagePair;

#pragma region Queue
typedef struct queue
{
	unsigned int tail;
	unsigned int head;
	unsigned int cnt;
	IdMessagePair data[RING_SIZE];
}RingBufferQueue;

IdMessagePair Dequeue(RingBufferQueue* queue)
{
	if (queue->cnt == 0)
	{
		char mess[] = "Empty queue";
		IdMessagePair idm = { -1, ""};
		strcpy_s(idm.message, DEFAULT_BUFLEN, mess);
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

	if (queue->cnt == 0)
	{
		return -1;
	}

	return (float)(queue->cnt) / RING_SIZE * 100;
}
#pragma endregion

#pragma region HashSet
typedef struct hash_item
{
	int index;
	ClientListItem* clientInfo;
	struct hash_item* next;
}HashItem;

typedef struct hash_set
{
	int size;
	HashItem** hashlist;
}HashSet;

HashSet* createHashSet(int size)
{
	HashSet* hs;
	hs = (HashSet*)malloc(sizeof(HashSet));

	if (hs == NULL)
	{
		return NULL;
	}

	hs->size = size;
	hs->hashlist = (HashItem**)calloc(size, sizeof(HashItem));

	if (hs->hashlist == NULL)
	{
		free(hs);
		return NULL;
	}

	return hs;
}

HashItem* findNodeHash(HashSet* hs, int index, int clientId)
{
	if (hs == NULL)
	{
		return NULL;
	}

	HashItem* hi = hs->hashlist[index];
	while (hi != NULL)
	{
		if (hi->index == index && hi->clientInfo->id == clientId)
		{
			return hi;
		}
		hi = hi->next;
	}
	return NULL;
}

bool insertHashItem(HashSet* hs, int index, ClientListItem* client)
{
	if (hs == NULL)
	{
		return false;
	}

	HashItem* current = hs->hashlist[index];

	while (current->next != NULL)
	{
		current = current->next;
	}

	current->next = (HashItem*)malloc(sizeof(HashItem));
	if (current->next == NULL)
	{
		return false;
	}
	current->next->clientInfo = client;
	current->next->index = index;
	current->next->next = NULL;
	return true;
}

bool deleteHashItem(HashSet* hs, int index, int clientId)

{
	if (hs == NULL)
	{
		return false;
	}

	HashItem* current = hs->hashlist[index];
	HashItem* temp = current;

	while (current != NULL)
	{
		if (current->clientInfo->id == clientId)
		{
			/*free(current->clientInfo);*/
			temp->next = current->next;
			free(current);
			return true;
		}
		temp = current;
		current = current->next;
	}
	return false;
}

void destroyHashSet(HashSet* hs)
{
	HashItem* hi;
	HashItem* temp;

	for (int i = 0; i < hs->size; i++)
	{
		hi = hs->hashlist[i];
		while (hi != NULL)
		{
			/*free(hi->clientInfo);*/
			temp = hi;
			hi = hi->next;
			free(temp);
		}
	}

	free(hs);
}
#pragma endregion

#pragma region WorkerRoleList
typedef struct list_item
{
	WorkerRole* wr;
	struct list_item* next;
}ListItem;

typedef struct list
{
	int listCounter;
	ListItem* head;
	ListItem* tail;
}List;

bool add(List* list, WorkerRole* wr) // at end
{
	ListItem* newItem = (ListItem*)malloc(sizeof(ListItem));

	if (newItem == NULL)
	{
		return false;
	}

	newItem->wr = wr;
	newItem->next = NULL;

	if (list->head == NULL)
	{
		list->head = newItem;
		list->tail = newItem;
	}
	else
	{
		list->tail->next = newItem;
		list->tail = newItem;
	}
	list->listCounter++;
	return true;
}

bool remove(List* list, int index) // by index
{
	ListItem* previous = NULL;
	ListItem* current = list->head;

	while (current != NULL && current->wr->id != index)
	{
		previous = current;
		current = current->next;
	}

	if (current != NULL)
	{
		if (previous == NULL) //First
		{
			list->head = current->next;
		}
		else
		{
			previous->next = current->next;
		}

		if (current == list->tail)
		{
			list->tail = previous; //Last
		}

		free(current);
		list->listCounter--;
		return true;
	}
	else
	{
		return false;
	}
}

bool move(List* src, List* dest, ListItem* item) // from list src to list dest move item
{
	ListItem* previous = NULL;
	ListItem* current = src->head;

	while (current != NULL && current != item)
	{
		previous = current;
		current = current->next;
	}

	if (current != NULL)
	{
		if (previous == NULL)
		{
			src->head = current->next; //First in src
		}
		else
		{
			previous->next = current->next;
		}

		if (current == src->tail)
		{
			src->tail = previous; //Last in src
		}

		dest->tail->next = current;
		dest->tail = current;
		current->next = NULL;
		return true;
	}
	else
	{
		return false; //Shouldn't happen, but you never know
	}
}

ListItem* find(List* list, int index) // find node by index
{
	ListItem* previous = NULL;
	ListItem* current = list->head;

	while (current != NULL && current->wr->id != index)
	{
		previous = current;
		current = current->next;
	}

	if (current != NULL)
	{
		return current;
	}
	else
	{
		return NULL;
	}
}

void deleteList(List* list)
{
	ListItem* current = list->head;
	ListItem* next;
	while (current != NULL)
	{
		next = current->next;
		free(current->wr);
		free(current);
		current = next;
	}

	list->head = NULL;
	list->tail = NULL;
	list->listCounter = 0;
}
#pragma endregion

#pragma region OTFun
typedef struct otparam
{
	RingBufferQueue* queue;
	List* workerList;
}OTParam;
#pragma endregion

#pragma region WTFun
typedef struct wtparam
{
	RingBufferQueue* queue;
	ClientListItem* client;
	CRITICAL_SECTION cs;
}WTParam;
#pragma endregion

#pragma region DTFun
typedef struct dtparam
{
	RingBufferQueue* queue;
	List* freeWorkerRole;
	List* busyWorkerRole;
}DTParam;
#pragma endregion

#pragma region WRFun
typedef struct wrparam
{
	ListItem* worker;
	HashSet* hs;
}WRParam;
#pragma endregion

#pragma region CMTFun
typedef struct cmtparam
{
	RingBufferQueue* queue;
	CRITICAL_SECTION cs;
	HashSet* hashSet;
}CMTParam;
#pragma endregion

#pragma region WMTFun
typedef struct wmtparam
{
	List* freeWorkerRoles;
	List* busyWorkerRoles;
	HashSet* hashSet;
	CRITICAL_SECTION cs;
}WMTParam;
#pragma endregion
#endif