#include "HashSet.h"

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