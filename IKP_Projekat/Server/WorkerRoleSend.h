#pragma once
#ifndef WS_H
#define WS_H

#include "../Common/Structs.h"

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

#endif // !WS_H
