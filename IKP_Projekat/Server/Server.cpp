#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WINDOWS_IGNORE_PACKING_MISMATCH

#include "ThreadFunctions.h"
#pragma pack(1)

CRITICAL_SECTION bufferQueue;
int main(void)
{    
    DWORD CMTID, OTID, DTID, WMTID;
    HANDLE CMT, OT, DT, WMT;
    HashSet* hashSet = createHashSet(MAX_CLIENTS);
    RingBufferQueue* queue = (RingBufferQueue*)malloc(sizeof(RingBufferQueue));
    queue->cnt = 0;
    queue->head = 0;
    queue->tail = 0;
    List* freeWorkerRoles = (List*)malloc(sizeof(List));
    freeWorkerRoles->listCounter = 0;
    freeWorkerRoles->head = NULL;
    freeWorkerRoles->tail = NULL;
    List* busyWorkerRoles = (List*)malloc(sizeof(List));
    busyWorkerRoles->listCounter = 0;
    busyWorkerRoles->head = NULL;
    busyWorkerRoles->tail = NULL;
    InitializeCriticalSection(&bufferQueue);

    WMTParam* wmtparam = (WMTParam*)malloc(sizeof(WMTParam));
    wmtparam->busyWorkerRoles = busyWorkerRoles;
    wmtparam->freeWorkerRoles = freeWorkerRoles;
    wmtparam->hashSet = hashSet;
    wmtparam->cs = bufferQueue;

    OTParam* otparam = (OTParam*)malloc(sizeof(OTParam));
    otparam->workerList = freeWorkerRoles;
    otparam->queue = queue;

    DTParam* dtparam = (DTParam*)malloc(sizeof(DTParam));
    dtparam->busyWorkerRole = busyWorkerRoles;
    dtparam->freeWorkerRole = freeWorkerRoles;
    dtparam->queue = queue;

    CMTParam* cmtparam = (CMTParam*)malloc(sizeof(CMTParam));
    cmtparam->cs = bufferQueue;
    cmtparam->hashSet = hashSet;
    cmtparam->queue = queue;

    WMT = CreateThread(NULL, 0, &WMTFunction, wmtparam, 0, &WMTID);   

    OT = CreateThread(NULL, 0, &OTFun, otparam, 0, &OTID);

    DT = CreateThread(NULL, 0, &DTFun, dtparam, 0, &DTID);  

    CMT = CreateThread(NULL, 0, &CMTFunction, cmtparam, 0, &CMTID);

    HANDLE mainHandles[4] = { CMT, OT, DT, WMT };

    WaitForMultipleObjects(4, mainHandles, TRUE, INFINITE);

    WSACleanup();
    CloseHandle(CMT);
    CloseHandle(OT);
    CloseHandle(DT);
    CloseHandle(WMT);
    destroyHashSet(hashSet);
    free(queue);
    deleteList(freeWorkerRoles);
    deleteList(busyWorkerRoles);
    DeleteCriticalSection(&bufferQueue);
    free(cmtparam);
    free(otparam);
    free(dtparam);
    free(wmtparam);
    return 0;
}