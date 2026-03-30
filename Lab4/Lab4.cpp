#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

const int MAX_GUESTS = 10;
const int PAINTINGS = 5;
const int MAX_AT_PAINTING = 10;
const int TOTAL_VISITORS = 30;

const char* SEM_GALLERY_NAME = "Gallery_Sem_Gallery";
const char* SHARED_MEM_NAME = "Gallery_SharedMem";

// Имена для sem_painting[i] и mutex_painting[i] строятся динамически:
// "Gallery_Sem_Painting_0" .. "Gallery_Sem_Painting_4"
// "Gallery_Mutex_Painting_0" .. "Gallery_Mutex_Painting_4"

// Разделяемая память вместо глобального массива
// (у каждого процесса своё адресное пространство)
struct SharedData {
    int visitors_at_painting[PAINTINGS];
};

// =========================================================
// Роль посетителя — запускается как отдельный процесс:
// gallery.exe visitor <id> <good: 0|1>
// =========================================================
int run_visitor(int id, bool good) {
    const char* type = good ? "Good" : "Windy";

    srand(GetTickCount() ^ (id * 1013));

    // Открываем уже созданные главным процессом объекты
    HANDLE sem_gallery = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, SEM_GALLERY_NAME);

    HANDLE sem_painting[PAINTINGS];
    HANDLE mutex_painting[PAINTINGS];
    for (int i = 0; i < PAINTINGS; i++) {
        char name[64];
        sprintf(name, "Gallery_Sem_Painting_%d", i);
        sem_painting[i] = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, name);
        sprintf(name, "Gallery_Mutex_Painting_%d", i);
        mutex_painting[i] = OpenMutex(MUTEX_ALL_ACCESS, FALSE, name);
    }

    HANDLE hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHARED_MEM_NAME);
    SharedData* shared = (SharedData*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedData));

    // Логика посетителя — один в один как VisitorThread из оригинала
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
        shared->visitors_at_painting[pic]++;
        printf("[%s %2d] is looking at the painting %d (it has %d visitors around it)\n",
            type, id, pic, shared->visitors_at_painting[pic]);
        ReleaseMutex(mutex_painting[pic]);

        Sleep(500 + rand() % 500);

        WaitForSingleObject(mutex_painting[pic], INFINITE);
        shared->visitors_at_painting[pic]--;
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

    UnmapViewOfFile(shared);
    CloseHandle(hMap);
    CloseHandle(sem_gallery);
    for (int i = 0; i < PAINTINGS; i++) {
        CloseHandle(sem_painting[i]);
        CloseHandle(mutex_painting[i]);
    }

    return 0;
}

// =========================================================
// Главный процесс — создаёт объекты и запускает дочерние
// =========================================================
int main(int argc, char* argv[]) {

    // Если запущены как дочерний процесс — идём в роль посетителя
    if (argc == 4 && strcmp(argv[1], "visitor") == 0) {
        return run_visitor(atoi(argv[2]), atoi(argv[3]) != 0);
    }

    // --- Создаём именованные объекты синхронизации ---
    HANDLE sem_gallery = CreateSemaphore(NULL, MAX_GUESTS, MAX_GUESTS, SEM_GALLERY_NAME);

    HANDLE sem_painting[PAINTINGS];
    HANDLE mutex_painting[PAINTINGS];
    for (int i = 0; i < PAINTINGS; i++) {
        char name[64];
        sprintf(name, "Gallery_Sem_Painting_%d", i);
        sem_painting[i] = CreateSemaphore(NULL, MAX_AT_PAINTING, MAX_AT_PAINTING, name);
        sprintf(name, "Gallery_Mutex_Painting_%d", i);
        mutex_painting[i] = CreateMutex(NULL, FALSE, name);
    }

    // --- Разделяемая память вместо глобального массива ---
    HANDLE hMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL,
        PAGE_READWRITE, 0, sizeof(SharedData), SHARED_MEM_NAME);
    SharedData* shared = (SharedData*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedData));
    memset(shared, 0, sizeof(SharedData));

    // --- Запускаем процессы-посетители ---
    char exe_path[MAX_PATH];
    GetModuleFileNameA(NULL, exe_path, MAX_PATH);

    srand(GetTickCount());

    HANDLE procs[TOTAL_VISITORS];
    for (int i = 0; i < TOTAL_VISITORS; i++) {
        bool good = (rand() % 2 == 0);

        char cmd[MAX_PATH + 64];
        sprintf(cmd, "\"%s\" visitor %d %d", exe_path, i, (int)good);

        STARTUPINFOA si = {};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi = {};

        CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        CloseHandle(pi.hThread);
        procs[i] = pi.hProcess;

        Sleep(200 + rand() % 100);
    }

    // WaitForMultipleObjects берёт максимум 64 объекта,
    // поэтому ждём по одному — для учебной задачи достаточно
    for (int i = 0; i < TOTAL_VISITORS; i++) {
        WaitForSingleObject(procs[i], INFINITE);
        CloseHandle(procs[i]);
    }

    printf("The gallery has been closed.\n");

    UnmapViewOfFile(shared);
    CloseHandle(hMap);
    CloseHandle(sem_gallery);
    for (int i = 0; i < PAINTINGS; i++) {
        CloseHandle(sem_painting[i]);
        CloseHandle(mutex_painting[i]);
    }

    return 0;
}