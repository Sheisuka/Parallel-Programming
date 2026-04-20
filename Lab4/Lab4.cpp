#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>

const int MAX_GUESTS = 5;
const int PAINTINGS = 5;
const int MAX_AT_PAINTING = 5;

struct SharedState {
    int visitors_at_painting[PAINTINGS];
    int gallery_counter;
    int initialized;
};

int main(int argc, char* argv[]) {
    int id = atoi(argv[1]);
    srand((unsigned)(id));
    bool good = (rand() % 2 == 0);
    const char* type = good ? "Good " : "Windy";

    HANDLE sem_gallery = CreateSemaphoreA(NULL, MAX_GUESTS, MAX_GUESTS, "Gallery_SemGallery");

    HANDLE sem_painting[PAINTINGS];
    HANDLE mutex_painting[PAINTINGS];
    char name[64];
    for (int i = 0; i < PAINTINGS; i++) {
        sprintf_s(name, "Gallery_SemPainting_%d", i);
        sem_painting[i] = CreateSemaphoreA(NULL, MAX_AT_PAINTING, MAX_AT_PAINTING, name);
        sprintf_s(name, "Gallery_MutexPainting_%d", i);
        mutex_painting[i] = CreateMutexA(NULL, FALSE, name);
    }
    HANDLE mutex_shared = CreateMutexA(NULL, FALSE, "Gallery_MutexShared");

    HANDLE hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedState), "Gallery_SharedState");
    SharedState* state = (SharedState*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedState));

    WaitForSingleObject(mutex_shared, INFINITE);
    if (!state->initialized) {
        for (int i = 0; i < PAINTINGS; i++) {
            state->visitors_at_painting[i] = 0;
        }
        state->gallery_counter = 0;
        state->initialized = 1;
    }
    ReleaseMutex(mutex_shared);


    WaitForSingleObject(sem_gallery, INFINITE);

    WaitForSingleObject(mutex_shared, INFINITE);
    ReleaseMutex(mutex_shared);
    printf("[%s %2d] Entered the gallery. At the gallery right now: %d\n",type, id, ++state->gallery_counter);

    int order[PAINTINGS];
    for (int i = 0; i < PAINTINGS; i++) order[i] = i;
    if (!good) {
        for (int i = PAINTINGS - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            int tmp = order[i];
            order[i] = order[j];
            order[j] = tmp;
        }
    }

    int viewed = 0;
    for (int step = 0; step < PAINTINGS; step++) {
        int pic = order[step];
        WaitForSingleObject(sem_painting[pic], INFINITE);

        WaitForSingleObject(mutex_painting[pic], INFINITE);
        printf("[%s %2d] is looking at the painting %d (it has %d visitors around it)\n", type, id, pic, ++state->visitors_at_painting[pic]);
        ReleaseMutex(mutex_painting[pic]);

        Sleep(500 + rand() % 500);

        WaitForSingleObject(mutex_painting[pic], INFINITE);
        --state->visitors_at_painting[pic];
        printf("[%s %2d] Went away off the painting %d\n", type, id, pic);
        ReleaseMutex(mutex_painting[pic]);

        ReleaseSemaphore(sem_painting[pic], 1, NULL);

        ++viewed;
        if (!good && (rand() % 3 == 0)) {
            printf("[%s %2d] Went away after seeing %d paintings.\n", type, id, viewed);
            break;
        }
    }
    if (viewed == PAINTINGS) {
        printf("[%s %2d] Went away after seeing all the paintings.\n", type, id);
    }

    WaitForSingleObject(mutex_shared, INFINITE);
    --state->gallery_counter;
    ReleaseMutex(mutex_shared);
    ReleaseSemaphore(sem_gallery, 1, NULL);

    UnmapViewOfFile(state);
    CloseHandle(hMap);
    CloseHandle(mutex_shared);
    for (int i = 0; i < PAINTINGS; i++) {
        CloseHandle(sem_painting[i]);
        CloseHandle(mutex_painting[i]);
    }
    CloseHandle(sem_gallery);
    return 0;
}