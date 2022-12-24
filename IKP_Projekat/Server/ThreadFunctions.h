#pragma once

#ifndef THREADFNC_H
#define THREADFNC_H

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#include "../Common/Structs.h"

bool InitializeWindowsSockets()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return false;
    }
    return true;
}

SOCKET connectTo(short port)
{
    SOCKET listenSocket = INVALID_SOCKET;
    int iResult;

    if (InitializeWindowsSockets() == false)
    {
        return 1;
    }

    sockaddr_in serverAddress;
    memset((char*)&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listenSocket == INVALID_SOCKET)
    {
        printf("Socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    iResult = bind(listenSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    if (iResult == SOCKET_ERROR)
    {
        printf("Bind failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    unsigned long  mode = 1;
    if (ioctlsocket(listenSocket, FIONBIO, &mode) != 0)
        printf("ioctlsocket failed with error.");

    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        printf("Listen failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    if (port == 5058)
        printf("Listening for incoming WORKER ROLE connections on port %d...\n", port);
    else if (port == 5059)
        printf("Listening for incoming CLIENT connections on port %d...\n", port);

    return listenSocket;
}

//CLIENT WRITE THREAD!
DWORD WINAPI handleClientRecieve(LPVOID params) //Kada primimo poruku od klijenta
{
    int iResult;

    WTParam* parameters = (WTParam*)params;
    ClientListItem* clientItem = parameters->client;
    SOCKET clientSocket = clientItem->socket;
    int clientId = clientItem->id;
    char* clientMessage = clientItem->request_message;

    while(true)
    {
        iResult = recv(clientSocket, clientMessage, DEFAULT_BUFLEN, 0);
        clientMessage[iResult] = '\0';

        IdMessagePair clientMessagePair = { clientId,""};
        strcpy_s(clientMessagePair.message, DEFAULT_BUFLEN, clientMessage);

        EnterCriticalSection(&parameters->cs);
        Enqueue(parameters->queue, clientMessagePair);
        LeaveCriticalSection(&parameters->cs);
    }
}

//SLANJE WORKER ROLI
DWORD WINAPI handleWorkerRoleSend(LPVOID params)
{
    int iResult;
    ListItem* worker = (ListItem*)params;
    while (true)
    {
        WaitForSingleObject(worker->wr->semaphore, INFINITE);

        iResult = send(worker->wr->socket, worker->wr->message_box, (int)strlen(worker->wr->message_box), 0);
        if (iResult == SOCKET_ERROR)
        {
            printf("Sending to WorkerRole(%d) failed with error: %d\n", worker->wr->id, WSAGetLastError());
            return 1;
        }
        printf("Bytes Sent To WorkerRole(%d): %ld\n", worker->wr->id, iResult);
    }
}

//SALJEMO SA WORKER ROLE ODOGOVARAJUCEM KLIJENTU - MAYBE CHANGE
DWORD WINAPI handleWorkerRoleReceive(LPVOID params)
{
    int iResult;   
    int sResult;
    WRParam* parameters = (WRParam*)params;
    ListItem* worker = parameters->worker;
    HashSet* hs = parameters->hs;

    while (true)
    {        
        iResult = recv(worker->wr->socket, worker->wr->message_box, DEFAULT_BUFLEN, 0);

        if (iResult > 1)
        {
            IdMessagePair* clientInfo = (IdMessagePair*)worker->wr->message_box;
            HashItem* client = findNodeHash(hs, clientInfo->clientId % hs->size, clientInfo->clientId);

            sResult = send(client->clientInfo->socket, clientInfo->message, DEFAULT_BUFLEN, 0);

            printf("Sending back data CLIENT #%d", clientInfo->clientId);

            if (sResult == SOCKET_ERROR)
            {
                printf("Sending data to CLIENT #%d failed\n", clientInfo->clientId);
            }
        }
        //NEED MAYBE FIX
        else
        {
            printf("Error receiving done message from WR %d:", worker->wr->id);
            closesocket(worker->wr->socket);
            WSACleanup();
        }
    }
}

//OBSERVER THREAD - LITTLE CHANGE NEEDED MAYBE
DWORD WINAPI OTFun(LPVOID params)
{
    while (true)
    {
        OTParam* parameters = (OTParam*)params;
        RingBufferQueue* queue = parameters->queue;
        List* freeWorkerRoles = parameters->workerList;
        float capacity = Capacity(queue);

        if (capacity > 70)
        {
            system("../../WorkerRole/Debug/WorkerRole.exe");
        }
        else if (capacity < 30 && freeWorkerRoles->listCounter > 1)
        {
            int id = freeWorkerRoles->head->wr->id;
            SOCKET s = freeWorkerRoles->head->wr->socket;
            char message[5] = "quit";
            strcpy_s(freeWorkerRoles->head->wr->message_box, DEFAULT_BUFLEN, message);
            //Mozda ovo prebaciti u WMT kada ga implementiramo!
            if (!remove(freeWorkerRoles, id))
            {
                printf("ERROR: Something is wrong with removing first element of free Worker Roles!!!");
            }
            printf("Successfully turned off Worker Role instance with id=%d.", id);
            ReleaseSemaphore(freeWorkerRoles->head->wr->semaphore, 1, NULL);
        }
        Sleep(5000);
    }
}

//DISPATCH THREAD
DWORD WINAPI DTFun(LPVOID param)
{
    DTParam* parameters = (DTParam*)param;
    RingBufferQueue* queue = parameters->queue;
    List* freeWorkersList = parameters->freeWorkerRole;
    List* busyWorkerList = parameters->busyWorkerRole;    
    while (true)
    {
        ListItem* freeWorker = find(freeWorkersList, freeWorkersList->head->wr->id);

        EnterCriticalSection(&freeWorker->wr->cs);
        IdMessagePair messagePair = Dequeue(queue);
        LeaveCriticalSection(&freeWorker->wr->cs);

        char message[DEFAULT_BUFLEN];
        strcpy_s(message, DEFAULT_BUFLEN, messagePair.clientId + messagePair.message);
        freeWorker->wr->busy = true;
        strcpy_s(freeWorker->wr->message_box, DEFAULT_BUFLEN, message);

        move(freeWorkersList, busyWorkerList, freeWorker);

        ReleaseSemaphore(freeWorker->wr->semaphore, 1, NULL);
    }    
}
#endif