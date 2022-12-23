#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "handleClientMessage.h"
#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512

void handleClient(SOCKET clientSocket, char* dataBuffer)
{
    int iResult;
    int iSendResult;

    do
    {
        iResult = recv(clientSocket, dataBuffer, DEFAULT_BUFLEN, 0);

        if (iResult > 0)
        {
            printf("Received %d bytes from client: %s\n", iResult, dataBuffer);

            iSendResult = send(clientSocket, dataBuffer, iResult, 0);

            if (iSendResult == SOCKET_ERROR)
            {
                printf("Send failed with error: %d\n", WSAGetLastError());
                closesocket(clientSocket);
                WSACleanup();
                return;
            }

            printf("Sent %d bytes back to client\n", iSendResult);
        }        
    } while (iResult > 0);
}