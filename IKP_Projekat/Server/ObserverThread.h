#pragma once
#ifndef OT_H
#define OT_H

#include "../Common/Structs.h"

//OBSERVER THREAD
DWORD WINAPI OTFun(LPVOID params)
{
    while (true)
    {
        OTParam* parameters = (OTParam*)params;
        float capacity = Capacity(parameters->queue);
        printf("Trenutan broj worker rola: %d\n", parameters->workerList->listCounter);

        if (capacity > 70)
        {
            const char* ime_programa = "D:\\fax\\Zimski semestar\\Industrijski_komunikacioni_protokoli\\Project\\Projekat\\ikp_project\\IKP_Projekat\\Debug\\WorkerRole.exe";
            int duzina = MultiByteToWideChar(CP_ACP, 0, ime_programa, -1, NULL, 0);
            wchar_t* ime_programa_wide = (wchar_t*)malloc(duzina * sizeof(wchar_t));
            MultiByteToWideChar(CP_ACP, 0, ime_programa, -1, ime_programa_wide, duzina);

            STARTUPINFO si;
            PROCESS_INFORMATION pi;

            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));

            // Pokretanje programa.exe
            if (CreateProcess(ime_programa_wide,  // Ime programa
                NULL,                        // Komandna linija
                NULL,                        // Proces specifikacije
                NULL,                        // Tred specifikacije
                FALSE,                       // Ne koristi se handle
                0,                           // Bez specijalnog pokretanja
                NULL,                        // Nekoristi se okruzenje
                NULL,                        // Nekoristi se radni direktorijum
                &si,                         // Struktura STARTUPINFO
                &pi))

                free(ime_programa_wide);
        }
        else if (capacity < 30 && parameters->workerList->listCounter > 1)
        {
            int id = parameters->workerList->head->wr->id;
            SOCKET s = parameters->workerList->head->wr->socket;
            char message[5] = "quit";
            strcpy_s(parameters->workerList->head->wr->message_box, DEFAULT_BUFLEN, message);
            //Mozda ovo prebaciti u WMT kada ga implementiramo!
            if (!remove(parameters->workerList, id))
            {
                printf("ERROR: Something is wrong with removing first element of free Worker Roles!!!");
            }
            printf("Successfully turned off Worker Role instance with id=%d.\n", id);
            ReleaseSemaphore(parameters->workerList->head->wr->semaphore, 1, NULL);
        }
        Sleep(5000);
    }
}
#endif // !OT_H
