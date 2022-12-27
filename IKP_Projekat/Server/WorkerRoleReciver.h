#pragma once
#ifndef WR_H
#define WR_H

#include "../Common/Structs.h"

//SALJEMO SA WORKER ROLE ODOGOVARAJUCEM KLIJENTU
DWORD WINAPI handleWorkerRoleReceive(LPVOID params)
{
	int iResult;
	int sResult;
	WRParam* parameters = (WRParam*)params;
	ListItem* worker = parameters->worker;
	HashSet* hs = parameters->hs;

	while (true)
	{
		WaitForSingleObject(worker->wr->semaphore[1], INFINITE);

		IdMessagePair* clientInfo = (IdMessagePair*)malloc(sizeof(IdMessagePair));
		sscanf_s(worker->wr->message_box, "%d ", &clientInfo->clientId);
		strcpy(clientInfo->message, worker->wr->message_box + 2);

		HashItem* client = findNodeHash(hs, clientInfo->clientId % hs->size, clientInfo->clientId);

		sResult = send(client->clientInfo->socket, clientInfo->message, (int)strlen(clientInfo->message), 0);

		printf("Sending back data CLIENT #%d: %s\n", clientInfo->clientId, clientInfo->message);

		if (sResult == SOCKET_ERROR)
		{
			printf("Sending data to CLIENT #%d failed\n", clientInfo->clientId);
		}
		free(clientInfo);
		memset(clientInfo->message, 0, DEFAULT_BUFLEN);
	}
	free(parameters);
}
#endif // !WR_H

