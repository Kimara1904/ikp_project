#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WINDOWS_IGNORE_PACKING_MISMATCH

#include "ThreadFunctions.h"
#pragma pack(1)

CRITICAL_SECTION bufferQueue;
int main(void)
{    
    DWORD CMTID, OTID, DTID, WMTID;
    /*HANDLE CMT, OT, DT, WMT;*/
    HashSet* hashSet = createHashSet(MAX_CLIENTS);
    RingBufferQueue* queue = (RingBufferQueue*)malloc(sizeof(RingBufferQueue));
    List* freeWorkerRoles = (List*)malloc(sizeof(List));
    freeWorkerRoles->head = NULL;
    freeWorkerRoles->tail = NULL;
    List* busyWorkerRoles = (List*)malloc(sizeof(List));
    busyWorkerRoles->head = NULL;
    busyWorkerRoles->tail = NULL;


    WSACleanup();
    /*CloseHandle(CMT);
    CloseHandle(OT);
    CloseHandle(DT);
    CloseHandle(WMT);*/
    destroyHashSet(hashSet);
    free(queue);
    deleteList(freeWorkerRoles);
    deleteList(busyWorkerRoles);
    DeleteCriticalSection(&bufferQueue);
    return 0;
}