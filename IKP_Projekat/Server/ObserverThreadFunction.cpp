#include "ObserverThreadFunction.h"
#include "../Common/Structs.h"

DWORD WINAPI OTFun(LPVOID params)
{
	while (true)
	{
		OTParam* parameters = (OTParam*)params;
		RingBufferQueue* queue = parameters->queue;
		List* freeWorkerRoles = parameters->workerList;
		float capacity = Capacity(queue);
		
		if (capacity > 70)
		{
			system("../../WorkerRole/Debug/WorkerRole.exe");
		}
		else if (capacity < 30 && freeWorkerRoles->listCounter > 1)
		{
			int id = freeWorkerRoles->head->wr->id;
			SOCKET s = freeWorkerRoles->head->wr->socket;
			char message[5] = "quit";
			handleWorkerRole(s, message);
			if (!remove(freeWorkerRoles, id))
			{
				printf("ERROR: Something is wrong with removing first element of free Worker Roles!!!");
			}
			printf("Successfully turned off Worker Role instance with id=%d.", id);
		}
		Sleep(5000);
	}
}