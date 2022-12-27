#pragma once
#ifndef COMMUNICATIONFUNCTION_H
#define COMMUNICATIONFUNCTION_H

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
#endif // !COMMUNICATIONFUNCTION_H

