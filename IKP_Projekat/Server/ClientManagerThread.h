#pragma once
#ifndef CMT_H
#define CMT_H

#include "../Common/Structs.h"
#include "CommunicationFunctions.h"
#include "WriterThread.h"

#define MAX_CLIENTS 5
#define DEFAULT_PORT_CLIENT 5059

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
#endif // !CMT_H
