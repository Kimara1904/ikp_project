#include "Structs.h"

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

HashSet* createHashSet(int size);
HashItem* findNodeHash(HashSet* hs, int index, int clientId);
bool insertHashItem(HashSet* hs, int index, ClientListItem* clent);
bool deleteHashItem(HashSet* hs, int index, int clientId);
void destroyHashSet(HashSet* hs);