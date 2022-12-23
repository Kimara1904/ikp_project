#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512

void handleWorkerRole(SOCKET workerRoleSocket, char* dataBuffer);