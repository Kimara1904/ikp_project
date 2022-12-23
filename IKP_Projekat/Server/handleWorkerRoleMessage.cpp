#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "handleWorkerRoleMessage.h"
#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512

void handleWorkerRole(SOCKET workerRoleSocket, char* dataBuffer)
{
    int iResult;

    do
    {        
        iResult = send(workerRoleSocket, dataBuffer, (int)strlen(dataBuffer) + 1, 0);

        if (iResult == SOCKET_ERROR)
        {
            printf("Send failed with error: %d\n", WSAGetLastError());
            closesocket(workerRoleSocket);
            WSACleanup();
            return;
        }

        memset(dataBuffer, 0, strlen(dataBuffer));

    } while (iResult > 1); //zato sto u dataBufferu i dalje ima '/0' tj ne mozemo ga skroz isprazniti
}