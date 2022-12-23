#include "WorerRoleList.h"

bool add(List* list, WorkerRole* wr)
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
	return true;
}

bool remove(List* list, int index)
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
		return true;
	}
	else
	{
		return false;
	}
}

bool move(List* src, List* dest, ListItem* item)
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

ListItem* find(List* list, int index)
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