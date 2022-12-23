#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "conio.h"
#include "connectHandler.h"
#include "handleClientMessage.h"
#include "handleWorkerRoleMessage.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#pragma pack(1)

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT_CLIENT 5059
#define DEFAULT_PORT_WR 5058
#define MAX_CLIENTS 5
#define MAX_WORKER_ROLE 5

int main(void)
{
    SOCKET listenSocket = INVALID_SOCKET;
    SOCKET listenSocketWR = INVALID_SOCKET;
    SOCKET clientSockets[MAX_CLIENTS];
    SOCKET workerRoleSockets[MAX_WORKER_ROLE];
    unsigned long  mode = 1;
    short lastIndex = 0;
    short lastIndexWR = 0;
    char dataBuffer[DEFAULT_BUFLEN];

    memset(clientSockets, 0, MAX_CLIENTS * sizeof(SOCKET));
    memset(workerRoleSockets, 0, MAX_WORKER_ROLE * sizeof(SOCKET));

    listenSocket = connectTo(DEFAULT_PORT_CLIENT);
    listenSocketWR = connectTo(DEFAULT_PORT_WR);

    if (listenSocket == INVALID_SOCKET || listenSocketWR == INVALID_SOCKET)
    {
        printf("Failed to connect to client or worker role.\n");
        return 1;
    }

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clientSockets[i] = INVALID_SOCKET;
    }

    for (int i = 0; i < MAX_WORKER_ROLE; i++)
    {
        workerRoleSockets[i] = INVALID_SOCKET;
    }

    fd_set readfds;
    fd_set readfdsWR;
    fd_set writefds; 
    timeval timeVal;
    timeVal.tv_sec = 1;
    timeVal.tv_usec = 0;

    printf("Waiting for incoming connections...\n");

    while (true)
    {
        FD_ZERO(&readfds);
        FD_ZERO(&readfdsWR);
        FD_ZERO(&writefds);

        if (lastIndex != MAX_CLIENTS)
        {
            FD_SET(listenSocket, &readfds);
        }

        if (lastIndexWR != MAX_WORKER_ROLE)
        {
            FD_SET(listenSocketWR, &readfdsWR);
        }

        for (int i = 0; i < lastIndex; i++)
        {
            FD_SET(clientSockets[i], &readfds);
        }

        for (int i = 0; i < lastIndexWR; i++)
        {
            FD_SET(workerRoleSockets[i], &writefds);
        }

        int selectResult = select(0, &readfds, NULL, NULL, &timeVal);
        int selectResultWR = select(0, &readfdsWR, &writefds, NULL, &timeVal);

        if (selectResult == SOCKET_ERROR)
        {
            printf("Select failed with error: %d\n", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }
        else if (selectResultWR == SOCKET_ERROR)
        {
            printf("SelectWR failed with error: %d\n", WSAGetLastError());
            closesocket(listenSocketWR);
            WSACleanup();
            return 1;
        }
        else if (selectResult == 0)
        {
            //printf("Timeout Expired!\n");
            continue;
        }
        else if (selectResultWR == 0)
        {
            //printf("TimeoutWR Expired!\n");
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

            for (int i = 0; i < lastIndex; i++)
            {
                if (FD_ISSET(clientSockets[i], &readfds))
                {
                    handleClient(clientSockets[i], dataBuffer);
                }
            }

            for (int i = 0; i < lastIndexWR; i++)
            {
                if (FD_ISSET(workerRoleSockets[i], &writefds))
                {
                    handleWorkerRole(workerRoleSockets[i], dataBuffer);
                }
            }
        }        
    }

    printf("Closing sockets.\n");

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        closesocket(clientSockets[i]);
    }

    for (int i = 0; i < MAX_WORKER_ROLE; i++)
    {
        closesocket(workerRoleSockets[i]);
    }

    closesocket(listenSocket);
    closesocket(listenSocketWR);

    WSACleanup();

    return 0;
}