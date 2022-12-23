#include "Structs.h"

typedef struct list_item
{
	WorkerRole* wr;
	struct list_item* next;
}ListItem;


typedef struct list
{
	ListItem* head;
	ListItem* tail;
}List;
//dodati tail
//implementirati zaseban remove
//bez free

bool add(List* list, WorkerRole* wr); // at end
bool remove(List* list, int index); // by index
bool move(List* src, List* dest, ListItem* item); // from list src to list dest move item
ListItem* find(List* list, int index); // find node by index