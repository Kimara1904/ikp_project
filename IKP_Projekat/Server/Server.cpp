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
    List* freeWorkerRoles = (List*)malloc(sizeof(List));
    freeWorkerRoles->listCounter = 0;
    freeWorkerRoles->head = NULL;
    freeWorkerRoles->tail = NULL;
    List* busyWorkerRoles = (List*)malloc(sizeof(List));
    busyWorkerRoles->listCounter = 0;
    busyWorkerRoles->head = NULL;
    busyWorkerRoles->tail = NULL;

    WMTParam* wmtparam = (WMTParam*)malloc(sizeof(WMTParam));
    wmtparam->busyWorkerRoles = busyWorkerRoles;
    wmtparam->freeWorkerRoles = freeWorkerRoles;
    wmtparam->hashSet = hashSet;
    wmtparam->cs = bufferQueue;

    WMT = CreateThread(NULL, 0, &WMTFunction, wmtparam, 0, &WMTID);

    OTParam* otparam = (OTParam*)malloc(sizeof(OTParam));
    otparam->workerList = freeWorkerRoles;
    otparam->queue = queue;

    OT = CreateThread(NULL, 0, &OTFun, otparam, 0, &OTID);

    DTParam* dtparam = (DTParam*)malloc(sizeof(DTParam));
    dtparam->busyWorkerRole = busyWorkerRoles;
    dtparam->freeWorkerRole = freeWorkerRoles;
    dtparam->queue = queue;

    DT = CreateThread(NULL, 0, &DTFun, dtparam, 0, &DTID);

    CMTParam* cmtparam = (CMTParam*)malloc(sizeof(CMTParam));
    cmtparam->cs = bufferQueue;
    cmtparam->hashSet = hashSet;
    cmtparam->queue = queue;

    CMT = CreateThread(NULL, 0, &CMTFunction, cmtparam, 0, &CMTID);

    

   

    

    

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