#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

const int MAX_GUESTS = 10;
const int PAINTINGS = 5;
const int MAX_AT_PAINTING = 10;

HANDLE sem_gallery;
HANDLE sem_painting[PAINTINGS];
HANDLE mutex_painting[PAINTINGS];

int visitors_at_painting[PAINTINGS] = { 0 };

struct Visitor {
    int id;
    bool good;
};

DWORD WINAPI VisitorThread(PVOID pvParam) {
    Visitor* p = (Visitor*)pvParam;
    int id = p->id;
    bool good = p->good;
    const char* type = good ? "Good" : "Windy";

    WaitForSingleObject(sem_gallery, INFINITE);
    printf("[%s %2d] Entered the gallery\n", type, id);

    int order[PAINTINGS];
    for (int i = 0; i < PAINTINGS; i++) order[i] = i;

    if (!good) {
        for (int i = PAINTINGS - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
        }
    }

    int viewed = 0;
    for (int step = 0; step < PAINTINGS; step++) {
        int pic = order[step];
        WaitForSingleObject(sem_painting[pic], INFINITE);

        WaitForSingleObject(mutex_painting[pic], INFINITE);
        visitors_at_painting[pic]++;
        printf("[%s %2d] is looking at the painting %d (it has %d visitors around it)\n", type, id, pic, visitors_at_painting[pic]);
        ReleaseMutex(mutex_painting[pic]);

        Sleep(500 + rand() % 500);

        WaitForSingleObject(mutex_painting[pic], INFINITE);
        visitors_at_painting[pic]--;
        printf("[%s %2d] Went away off the painting %d\n", type, id, pic);
        ReleaseMutex(mutex_painting[pic]);

        ReleaseSemaphore(sem_painting[pic], 1, NULL);

        viewed++;

        if (!good && (rand() % 3 == 0)) {
            printf("[%s %2d] Went away after seeing %d paintings.\n", type, id, viewed);
            break;
        }
    }

    printf("[%s %2d] Went away after seeing all the paintings.\n", type, id);
    ReleaseSemaphore(sem_gallery, 1, NULL);
    delete p;

    return 0;
}

int main() {
    sem_gallery = CreateSemaphore(NULL, MAX_GUESTS, MAX_GUESTS, NULL);
    for (int i = 0; i < PAINTINGS; i++) {
        sem_painting[i] = CreateSemaphore(NULL, MAX_AT_PAINTING, MAX_AT_PAINTING, NULL);
        mutex_painting[i] = CreateMutex(NULL, FALSE, NULL);
    }

    const int TOTAL_VISITORS = 30;
    HANDLE threads[TOTAL_VISITORS];
    for (int i = 0; i < TOTAL_VISITORS; i++) {
        Visitor* p = new Visitor;
        p->id = i;
        p->good = bool(rand() % 2 == 0);

        threads[i] = CreateThread(NULL, 0, VisitorThread, p, 0, NULL);

        if (!threads[i]) {
            printf("Failed to create thread %d\n", i);
        }

        Sleep(200 + rand() % 100);
    }

    WaitForMultipleObjects(TOTAL_VISITORS, threads, TRUE, INFINITE);

    printf("The gallery has been closed.\n");

    CloseHandle(sem_gallery);
    for (int i = 0; i < PAINTINGS; i++) {
        CloseHandle(sem_painting[i]);
        CloseHandle(mutex_painting[i]);
    }
    for (int i = 0; i < TOTAL_VISITORS; i++) {
        CloseHandle(threads[i]);
    }

    return 0;
}