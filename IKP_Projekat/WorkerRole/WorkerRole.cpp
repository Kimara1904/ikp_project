#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "../Common/Structs.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#pragma pack(1)

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 5058

bool InitializeWindowsSockets();

int __cdecl main(int argc, char** argv)
{
    SOCKET workerRoleSocket = INVALID_SOCKET;
    int iResult;
    int sResult;
    char dataBuffer[DEFAULT_BUFLEN];

    if (argc != 1)
    {
        printf("usage: %s server-name\n", argv[0]);
        return 1;
    }

    if (InitializeWindowsSockets() == false)
    {
        return 1;
    }

    workerRoleSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (workerRoleSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(DEFAULT_PORT);

    if (connect(workerRoleSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
    {
        printf("Unable to connect to server.\n");
        closesocket(workerRoleSocket);
        WSACleanup();
    }

    printf("Succesfully connected to server.\n");

    while (true)
    {        
        iResult = recv(workerRoleSocket, dataBuffer, DEFAULT_BUFLEN, 0);

        dataBuffer[iResult] = '\0';

        if (strcmp(dataBuffer, "quit") == 0)
        {
            //SLANJE QUIT WMT-U
            closesocket(workerRoleSocket);
            printf("Disconnected from server.");
            break;
        }
        if (iResult > 1)
        {
            //SLANJE PORUKE WMT-U I WR
            printf("Message received from server: %s\n", dataBuffer);

            //OBRADA PODATAKA

            IdMessagePair messagePair;

            sscanf_s(dataBuffer, "%d %s", &messagePair.clientId, messagePair.message, sizeof(messagePair.message));
            
            char processed[50] = "Done with request... sending it back...";
            strcat_s(processed, messagePair.message);

            sprintf_s(dataBuffer, "%d %s", messagePair.clientId, processed);
            sResult = send(workerRoleSocket, dataBuffer, (int)strlen(dataBuffer), 0);

            printf("Sending back data to loadBalancer. Request was originally from: client #%d", messagePair.clientId);

            if (sResult == SOCKET_ERROR)
            {
                printf("Sending processed data failed with error: %d\n", WSAGetLastError());
                closesocket(workerRoleSocket);
                WSACleanup();
                return 1;
            }
        }
    }

    closesocket(workerRoleSocket);
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