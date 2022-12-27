#pragma once
#ifndef DT_H
#define DT_H

#include "../Common/Structs.h"

//DISPATCH THREAD
DWORD WINAPI DTFun(LPVOID params)
{
    DTParam* parameters = (DTParam*)params;
    while (true)
    {
        //Sleep(27000); - ZA TESTIRANJE VISE WORKER ROLA DA SE GASE I PALE A PALE SE A BGM SE I GASE

        if (Capacity(parameters->queue) == 0)
        {
            continue;
        }

        if (parameters->freeWorkerRole->listCounter == 0)
        {
            continue;
        }

        ListItem* freeWorker = find(parameters->freeWorkerRole, parameters->freeWorkerRole->head->wr->id);

        EnterCriticalSection(&freeWorker->wr->cs);
        IdMessagePair messagePair = Dequeue(parameters->queue);
        LeaveCriticalSection(&freeWorker->wr->cs);

        char message[DEFAULT_BUFLEN];
        sprintf_s(message, "%d %s", messagePair.clientId, messagePair.message);
        strcpy_s(freeWorker->wr->message_box, DEFAULT_BUFLEN, message);

        move(parameters->freeWorkerRole, parameters->busyWorkerRole, freeWorker);

        ReleaseSemaphore(freeWorker->wr->semaphore[0], 1, NULL);
    }
}

#endif // !DT_H

