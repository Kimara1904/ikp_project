#pragma once

#ifndef WMT_H
#define WMT_H

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#include "../Common/Structs.h"
#include "CommunicationFunctions.h"
#include "WorkerRoleSend.h"
#include "WorkerRoleReciver.h"
#define DEFAULT_PORT_WR 5058
#define MAX_WORKER_ROLE 5

//Worker Role Manager Thread Function
DWORD WINAPI WMTFunction(LPVOID params)
{
    SOCKET listenSocketWR = INVALID_SOCKET;
    SOCKET workerRoleSockets[MAX_WORKER_ROLE];
    unsigned long  mode = 1;
    short lastIndexWR = 0;
    bool taken[MAX_WORKER_ROLE] = { false };
    char dataBuffer[DEFAULT_BUFLEN];
    DWORD WSID[MAX_WORKER_ROLE];
    HANDLE WSThreads[MAX_WORKER_ROLE];
    DWORD WRID[MAX_WORKER_ROLE];
    HANDLE WRThreads[MAX_WORKER_ROLE];
    HANDLE SemaphoreWR[MAX_WORKER_ROLE];
    HANDLE SemaphoreWS[MAX_WORKER_ROLE];
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