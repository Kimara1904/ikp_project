#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "conio.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#pragma pack(1)

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 5059

bool InitializeWindowsSockets();

int __cdecl main(int argc, char** argv)
{
    SOCKET connectSocket = INVALID_SOCKET;
    int iResult;
    char message[DEFAULT_BUFLEN];

    if (argc != 1)
    {
        printf("usage: %s server-name\n", argv[0]);
        return 1;
    }

    if (InitializeWindowsSockets() == false)
    {
        return 1;
    }

    connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (connectSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(DEFAULT_PORT);

    if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
    {
        printf("Unable to connect to server.\n");
        closesocket(connectSocket);
        WSACleanup();
    }

    while (true)
    {
        printf("Send a message to server:\n");
        fgets(message, sizeof(message), stdin);
        if ((strlen(message) > 0) && (message[strlen(message) - 1] == '\n'))
        {
            message[strlen(message) - 1] = '\0';
        }

        iResult = send(connectSocket, message, (int)strlen(message) + 1, 0);

        if (strcmp(message, "quit") == 0)
        {
            closesocket(connectSocket);
            printf("Disconnected from server.");
            exit(1);
        }

        if (iResult == SOCKET_ERROR)
        {
            printf("Send failed with error: %d\n", WSAGetLastError());
            closesocket(connectSocket);
            WSACleanup();
            return 1;
        }

        printf("Bytes Sent: %ld\n", iResult);

        iResult = recv(connectSocket, message, DEFAULT_BUFLEN, 0);

        message[iResult] = '\0';

        if (iResult > 0)
        {
            printf("Message received back from server: %s\n", message);
        }
        else
        {
            printf("Error receiving message from server.");
        }
    }

    closesocket(connectSocket);
    WSACleanup();

    return 0;
}

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