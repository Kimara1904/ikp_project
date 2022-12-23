#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include "conio.h"

typedef struct client_struct_list_item
{
	int id;
	SOCKET socket;
	char* request_message;
}ClientListItem;

//typedef struct client_struct_thread_item
//{
//	U SLUCAJU DA RAZDVAJAMO CLIENT STRUCT NA ONAJ U HASH SET-U I U THREAD-OVIMA
//}ClientThread;

typedef struct worker_role_struct
{
	int id;
	bool busy;
	SOCKET socket;
	char* message_box;
	CRITICAL_SECTION cs;
	HANDLE semaphore;
}WorkerRole;
// DA LI SOCKET, KRITICNE SEKCIJE I SEMAFORI TREBA DA BUDU POKAZIVACI???

typedef struct clientid_message_pair
{
	int clientId;
	char* message;
}IdMessagePair;