#pragma once
#ifndef WT_H
#define WT_H

#include "../Common/Structs.h"
//CLIENT WRITE THREAD!
DWORD WINAPI handleClientRecieve(LPVOID params) //Kada primimo poruku od klijenta
{

    WTParam* parameters = (WTParam*)params;
    int clientId = parameters->client->id;
    char* clientMessage = parameters->client->request_message;

    while (true)
    {
        WaitForSingleObject(parameters->client->semaphore, INFINITE);
        IdMessagePair clientMessagePair = { clientId,"" };
        strcpy_s(clientMessagePair.message, DEFAULT_BUFLEN, clientMessage);

        EnterCriticalSection(&parameters->cs);
        Enqueue(parameters->queue, clientMessagePair);
        LeaveCriticalSection(&parameters->cs);
    }
    free(parameters);
}
#endif // !WT_H

