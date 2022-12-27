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

    WTParam* parameters = (WTParam*)params;
    int clientId = parameters->client->id;
    char* clientMessage = parameters->client->request_message;

    while(true)
    {
        WaitForSingleObject(parameters->client->semaphore, INFINITE);
        IdMessagePair clientMessagePair = { clientId,""};
        strcpy_s(clientMessagePair.message, DEFAULT_BUFLEN, clientMessage);

        EnterCriticalSection(&parameters->cs);
        Enqueue(parameters->queue, clientMessagePair);
        LeaveCriticalSection(&parameters->cs);
    }
    free(parameters);
}

//SLANJE WORKER ROLI
DWORD WINAPI handleWorkerRoleSend(LPVOID params)
{
    int iResult;
    ListItem* worker = (ListItem*)params;
    while (true)
    {
        WaitForSingleObject(worker->wr->semaphore[0], INFINITE);

        iResult = send(worker->wr->socket, worker->wr->message_box, (int)strlen(worker->wr->message_box), 0);
        if (iResult == SOCKET_ERROR)
        {
            printf("Sending to WorkerRole(%d) failed with error: %d\n", worker->wr->id, WSAGetLastError());
            return 1;
        }
        printf("Bytes Sent To WorkerRole(%d): %ld\n", worker->wr->id, iResult);
    }
}

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

//OBSERVER THREAD
DWORD WINAPI OTFun(LPVOID params)
{
    while (true)
    {
        OTParam* parameters = (OTParam*)params;        
        float capacity = Capacity(parameters->queue);

        if (capacity > 70)
        {
            if (capacity > 70)
            {
                const char* ime_programa = "C:\\Users\\zdrav\\Desktop\\ikp_project\\IKP_Projekat\\x64\\Debug\\WorkerRole.exe";
                int duzina = MultiByteToWideChar(CP_ACP, 0, ime_programa, -1, NULL, 0);
                wchar_t* ime_programa_wide = (wchar_t*)malloc(duzina * sizeof(wchar_t));
                MultiByteToWideChar(CP_ACP, 0, ime_programa, -1, ime_programa_wide, duzina);

                STARTUPINFO si;
                PROCESS_INFORMATION pi;

                ZeroMemory(&si, sizeof(si));
                si.cb = sizeof(si);
                ZeroMemory(&pi, sizeof(pi));

                // Pokretanje programa.exe
                if (CreateProcess(ime_programa_wide,  // Ime programa
                    NULL,                        // Komandna linija
                    NULL,                        // Proces specifikacije
                    NULL,                        // Tred specifikacije
                    FALSE,                       // Ne koristi se handle
                    0,                           // Bez specijalnog pokretanja
                    NULL,                        // Nekoristi se okruzenje
                    NULL,                        // Nekoristi se radni direktorijum
                    &si,                         // Struktura STARTUPINFO
                    &pi)) {                      // Struktura PROCESS_INFORMATION
                    parameters->workerList->listCounter++;
                }

                free(ime_programa_wide);
            }
        }
        else if (capacity < 30 && parameters->workerList->listCounter > 1)
        {
            int id = parameters->workerList->head->wr->id;
            SOCKET s = parameters->workerList->head->wr->socket;
            char message[5] = "quit";
            strcpy_s(parameters->workerList->head->wr->message_box, DEFAULT_BUFLEN, message);
            //Mozda ovo prebaciti u WMT kada ga implementiramo!
            if (!remove(parameters->workerList, id))
            {
                printf("ERROR: Something is wrong with removing first element of free Worker Roles!!!");
            }
            printf("Successfully turned off Worker Role instance with id=%d.", id);
            ReleaseSemaphore(parameters->workerList->head->wr->semaphore, 1, NULL);
        }
        Sleep(5000);
    }
}

//DISPATCH THREAD
DWORD WINAPI DTFun(LPVOID params)
{
    //Sleep(40000);
    DTParam* parameters = (DTParam*)params;     
    while (true)
    {

        if (Capacity(parameters->queue) == 0)
        {
            continue;
        }

        if (parameters->freeWorkerRole->listCounter == 0)
        {
            continue;
        }

        ListItem* freeWorker = find(parameters->freeWorkerRole, parameters->freeWorkerRole->head->wr->id);

        EnterCriticalSection(&freeWorker->wr->cs);
        IdMessagePair messagePair = Dequeue(parameters->queue);
        LeaveCriticalSection(&freeWorker->wr->cs);

        char message[DEFAULT_BUFLEN];
        sprintf_s(message, "%d %s", messagePair.clientId, messagePair.message);
        strcpy_s(freeWorker->wr->message_box, DEFAULT_BUFLEN, message);

        move(parameters->freeWorkerRole, parameters->busyWorkerRole, freeWorker);

        ReleaseSemaphore(freeWorker->wr->semaphore[0], 1, NULL);
    }    
}

//Client Manager Thread Function
DWORD WINAPI CMTFunction(LPVOID params)
{
    SOCKET listenSocket = INVALID_SOCKET;
    SOCKET clientSockets[MAX_CLIENTS];
    unsigned long  mode = 1;
    short lastIndex = 0;
    bool taken[MAX_CLIENTS] = { false };
    char dataBuffer[DEFAULT_BUFLEN];
    DWORD TID[MAX_CLIENTS];
    HANDLE threads[MAX_CLIENTS];
    HANDLE Semaphore[MAX_CLIENTS];
    CMTParam* parameters = (CMTParam*)params;
    ClientListItem* client = NULL;

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

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (taken[i])
            {
                FD_SET(clientSockets[i], &readfds);
            }
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
        else if (FD_ISSET(listenSocket, &readfds)) // Connect with client
        {
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (!taken[i])
                {
                    sockaddr_in clientAddr;
                    int clientAddrSize = sizeof(struct sockaddr_in);


                    clientSockets[i] = accept(listenSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);

                    if (clientSockets[i] == INVALID_SOCKET)
                    {
                        printf("Accept failed with error: %d\n", WSAGetLastError());
                    }
                    else
                    {
                        if (ioctlsocket(clientSockets[i], FIONBIO, &mode) != 0)
                        {
                            printf("ioctlsocket failed with error.");
                            continue;
                        }

                        Semaphore[i] = CreateSemaphore(NULL, 0, 1, NULL);

                        client = (ClientListItem*)malloc(sizeof(ClientListItem));
                        client->id = i;
                        client->socket = clientSockets[i];
                        client->semaphore = Semaphore[i];
                        strcpy_s(client->request_message, DEFAULT_BUFLEN, "");

                        insertHashItem(parameters->hashSet, i % MAX_HASH_LIST, client);

                        WTParam* wtp = (WTParam*)malloc(sizeof(WTParam));
                        wtp->client = client;
                        wtp->cs = parameters->cs;
                        wtp->queue = parameters->queue;

                        threads[lastIndex] = CreateThread(NULL, 0, &handleClientRecieve, wtp, 0, &TID[i]);
                        /*free(wtp);*/
                        taken[i] = true;
                        lastIndex++;
                        printf("New client #%d accepted. We have %d clients spaces left.\n", i, MAX_CLIENTS - lastIndex);
                        break;
                    }
                }
            }
            
        }
        else // Get a message
        {
            //proveravamo da li je poslat quit i gasimo threads ako dodje do toga
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (taken[i])
                {
                    if (FD_ISSET(clientSockets[i], &readfds))
                    {
                        int iResult = recv(clientSockets[i], dataBuffer, DEFAULT_BUFLEN, 0);
                        if (iResult <= 0)
                        {
                            continue;
                        }

                        dataBuffer[iResult] = '\0';

                        printf("From client we got message: %s\n", dataBuffer);

                        HashItem* item = findNodeHash(parameters->hashSet, i % MAX_HASH_LIST, i);

                        client = item->clientInfo;

                        if (strcmp(dataBuffer, "quit") == 0)
                        {
                            deleteHashItem(parameters->hashSet, i % MAX_HASH_LIST, i);
                            free(client);
                            CloseHandle(threads[i]);
                            CloseHandle(Semaphore[i]);
                            taken[i] = false;
                            lastIndex--;
                        }
                        else
                        {
                            strcpy(client->request_message, dataBuffer);
                            ReleaseSemaphore(client->semaphore, 1, NULL);
                        }
                    }
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
    bool taken[MAX_WORKER_ROLE] = { false };
    char dataBuffer[DEFAULT_BUFLEN];
    DWORD WSID[MAX_CLIENTS];
    HANDLE WSThreads[MAX_CLIENTS];
    DWORD WRID[MAX_CLIENTS];
    HANDLE WRThreads[MAX_CLIENTS];
    HANDLE SemaphoreWR[MAX_CLIENTS];
    HANDLE SemaphoreWS[MAX_CLIENTS];
    WMTParam* parameters = (WMTParam*)params;
    WorkerRole* worker = NULL;

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

        for (int i = 0; i < MAX_WORKER_ROLE; i++)
        {
            if (taken[i])
            {
                FD_SET(workerRoleSockets[i], &writefds);
            }            
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
        else if (FD_ISSET(listenSocketWR, &readfdsWR)) // Connect with Worker Role
        {
            for (int i = 0; i < MAX_WORKER_ROLE; i++)
            {
                if (!taken[i])
                {
                    sockaddr_in workerRoleAddr;
                    int workerRoleAddrSize = sizeof(struct sockaddr_in);

                    workerRoleSockets[i] = accept(listenSocketWR, (struct sockaddr*)&workerRoleAddr, &workerRoleAddrSize);

                    if (workerRoleSockets[i] == INVALID_SOCKET)
                    {
                        printf("Accept WR failed with error: %d\n", WSAGetLastError());
                    }
                    else
                    {
                        if (ioctlsocket(workerRoleSockets[i], FIONBIO, &mode) != 0)
                        {
                            printf("ioctlsocket failed with error.");
                            continue;
                        }

                        worker = (WorkerRole*)malloc(sizeof(WorkerRole));
                        worker->id = i;
                        worker->socket = workerRoleSockets[i];
                        worker->cs = parameters->cs;
                        SemaphoreWR[i] = CreateSemaphore(NULL, 0, 1, NULL);
                        SemaphoreWS[i] = CreateSemaphore(NULL, 0, 1, NULL);
                        worker->semaphore[0] = SemaphoreWS[i];
                        worker->semaphore[1] = SemaphoreWR[i];

                        add(parameters->freeWorkerRoles, worker);

                        WRParam* wrp = (WRParam*)malloc(sizeof(WRParam));
                        wrp->hs = parameters->hashSet;
                        wrp->worker = find(parameters->freeWorkerRoles, i);

                        WSThreads[i] = CreateThread(NULL, 0, &handleWorkerRoleSend, wrp->worker, 0, &WSID[i]);
                        WRThreads[i] = CreateThread(NULL, 0, &handleWorkerRoleReceive, wrp, 0, &WRID[i]);
                        taken[i] = true;
                        lastIndexWR++;

                        /*free(wrp);*/
                        printf("New worker role #%d assigned.\n", i);
                        break;
                    }
                }
            }
        }
        else // Get Message
        {
            for (int i = 0; i < MAX_WORKER_ROLE; i++)
            {
                if (taken[i])
                {
                    if (FD_ISSET(workerRoleSockets[i], &writefds))
                    {
                        int iResult = recv(workerRoleSockets[i], dataBuffer, DEFAULT_BUFLEN, 0);

                        if (iResult <= 0)
                        {
                            continue;
                        }

                        dataBuffer[iResult] = '\0';

                        ListItem* li = find(parameters->busyWorkerRoles, i);

                        worker = li->wr;

                        if (strcmp(dataBuffer, "quit") == 0)
                        {
                            remove(parameters->freeWorkerRoles, i);
                            free(worker);
                            CloseHandle(WSThreads[i]);
                            CloseHandle(WRThreads[i]);
                            CloseHandle(SemaphoreWS[i]);
                            CloseHandle(SemaphoreWR[i]);
                            taken[i] = false;
                            lastIndexWR--;
                        }
                        else
                        {
                            move(parameters->busyWorkerRoles, parameters->freeWorkerRoles, li);
                            printf("Worker Role %d is successfully moved to free!\n", i);
                            strcpy(worker->message_box, dataBuffer);
                            ReleaseSemaphore(worker->semaphore[1], 1, NULL);
                        }
                    }
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