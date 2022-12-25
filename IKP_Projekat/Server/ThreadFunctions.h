#pragma once

#ifndef THREADFNC_H
#define THREADFNC_H

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#include "../Common/Structs.h"
#define MAX_CLIENTS 5
#define DEFAULT_PORT_CLIENT 5059
#define DEFAULT_PORT_WR 5058
#define MAX_WORKER_ROLE 5

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

//Client Manager Thread Function
DWORD WINAPI CMTFunction(LPVOID params)
{
    SOCKET listenSocket = INVALID_SOCKET;
    SOCKET clientSockets[MAX_CLIENTS];
    unsigned long  mode = 1;
    short lastIndex = 0;
    char dataBuffer[DEFAULT_BUFLEN];

    memset(clientSockets, 0, MAX_CLIENTS * sizeof(SOCKET));
    listenSocket = connectTo(DEFAULT_PORT_CLIENT);

    if (listenSocket == INVALID_SOCKET)
    {
        printf("Failed to connect to client or worker role.\n");
        return 1;
    }

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clientSockets[i] = INVALID_SOCKET;
    }

    fd_set readfds;
    fd_set writefds;
    timeval timeVal;
    timeVal.tv_sec = 1;
    timeVal.tv_usec = 0;

    printf("Waiting for incoming connections...\n");

    while (true)
    {
        FD_ZERO(&readfds);
        if (lastIndex != MAX_CLIENTS)
        {
            FD_SET(listenSocket, &readfds);
        }

        for (int i = 0; i < lastIndex; i++)
        {
            FD_SET(clientSockets[i], &readfds);
        }

        int selectResult = select(0, &readfds, NULL, NULL, &timeVal);

        if (selectResult == SOCKET_ERROR)
        {
            printf("Select failed with error: %d\n", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }
        else if (selectResult == 0)
        {
            //printf("Timeout Expired!\n");
            continue;
        }

        else if (FD_ISSET(listenSocket, &readfds))
        {
            sockaddr_in clientAddr;
            int clientAddrSize = sizeof(struct sockaddr_in);

            clientSockets[lastIndex] = accept(listenSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);

            if (clientSockets[lastIndex] == INVALID_SOCKET)
            {
                printf("Accept failed with error: %d\n", WSAGetLastError());
            }
            else
            {
                if (ioctlsocket(clientSockets[lastIndex], FIONBIO, &mode) != 0)
                {
                    printf("ioctlsocket failed with error.");
                    continue;
                }
                lastIndex++;
                printf("New client #%d accepted. We have %d clients spaces left.\n", lastIndex, MAX_CLIENTS - lastIndex);
            }
        }
        else
        {
            for (int i = 0; i < lastIndex; i++)
            {
                if (FD_ISSET(clientSockets[i], &readfds))
                {
                    //NNEDS FIX
                    //handleClient(clientSockets[i], dataBuffer);
                }
            }
        }
    }

    printf("Closing sockets.\n");

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        closesocket(clientSockets[i]);
    }

    closesocket(listenSocket);
}

//Worker Role Manager Thread Function
DWORD WINAPI WMTFunction(LPVOID params)
{
    SOCKET listenSocketWR = INVALID_SOCKET;
    SOCKET workerRoleSockets[MAX_WORKER_ROLE];
    unsigned long  mode = 1;
    short lastIndexWR = 0;
    char dataBuffer[DEFAULT_BUFLEN];

    memset(workerRoleSockets, 0, MAX_WORKER_ROLE * sizeof(SOCKET));

    listenSocketWR = connectTo(DEFAULT_PORT_WR);

    if (listenSocketWR == INVALID_SOCKET)
    {
        printf("Failed to connect to client or worker role.\n");
        return 1;
    }

    for (int i = 0; i < MAX_WORKER_ROLE; i++)
    {
        workerRoleSockets[i] = INVALID_SOCKET;
    }

    fd_set readfdsWR;
    fd_set writefds;
    timeval timeVal;
    timeVal.tv_sec = 1;
    timeVal.tv_usec = 0;

    printf("Waiting for incoming connections...\n");

    while (true)
    {
        FD_ZERO(&readfdsWR);
        FD_ZERO(&writefds);



        if (lastIndexWR != MAX_WORKER_ROLE)
        {
            FD_SET(listenSocketWR, &readfdsWR);
        }



        for (int i = 0; i < lastIndexWR; i++)
        {
            FD_SET(workerRoleSockets[i], &writefds);
        }


        int selectResultWR = select(0, &readfdsWR, &writefds, NULL, &timeVal);


        if (selectResultWR == SOCKET_ERROR)
        {
            printf("SelectWR failed with error: %d\n", WSAGetLastError());
            closesocket(listenSocketWR);
            WSACleanup();
            return 1;
        }
        else if (selectResultWR == 0)
        {
            //printf("TimeoutWR Expired!\n");
            continue;
        }
        else if (FD_ISSET(listenSocketWR, &readfdsWR))
        {
            sockaddr_in workerRoleAddr;
            int workerRoleAddrSize = sizeof(struct sockaddr_in);

            workerRoleSockets[lastIndexWR] = accept(listenSocketWR, (struct sockaddr*)&workerRoleAddr, &workerRoleAddrSize);

            if (workerRoleSockets[lastIndexWR] == INVALID_SOCKET)
            {
                printf("Accept WR failed with error: %d\n", WSAGetLastError());
            }
            else
            {
                if (ioctlsocket(workerRoleSockets[lastIndexWR], FIONBIO, &mode) != 0)
                {
                    printf("ioctlsocket failed with error.");
                    continue;
                }
                lastIndexWR++;
                printf("New worker role #%d assigned.\n", lastIndexWR);
            }
        }
        else
        {
            for (int i = 0; i < lastIndexWR; i++)
            {
                if (FD_ISSET(workerRoleSockets[i], &writefds))
                {
                    //Ovde ce biti thread koji poziva handleWorkerRole funkiciju.                    
                    //handleWorkerRole(workerRoleSockets[i], dataBuffer);
                }
            }
        }
    }

    printf("Closing sockets.\n");



    for (int i = 0; i < MAX_WORKER_ROLE; i++)
    {
        closesocket(workerRoleSockets[i]);
    }


    closesocket(listenSocketWR);
}
#endif