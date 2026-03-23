#include <stdio.h>
#include <conio.h>
#include <windows.h>

const int P = 3;
const int N = 5;
const int M = 5;
const int CELLS = P * N * M;

int plan[P][N][M];
int garden[P][N][M];

int routePlot[2][CELLS];
int routeI[2][CELLS];
int routeJ[2][CELLS];
int routeLen[2];

int curPlot[2];
int curI[2];
int curJ[2];

int finished[2];

volatile int want[2];
volatile int turn = 0;

void Lock(int num) {
    int other = 1 - num;
    want[num] = 1;
    turn = other;
    while (want[other] && turn == other) {}
}

void Unlock(int num) {
    want[num] = 0;
}

// bottom -> top
void BuildRoute1() {
    int p, i, j, t = 0;
    for (p = 0; p < P; ++p) {
        for (i = 0; i < N; ++i) {
            for (j = 0; j < M; ++j) {
                routePlot[0][t] = p;
                routeI[0][t] = i;
                routeJ[0][t] = j;
                t++;
            }
        }
    }

    routeLen[0] = t;
}

// right -> left
void BuildRoute2() {
    int p, i, j, t = 0;
    for (p = 0; p < P; ++p)
        for (j = M - 1; j >= 0; j--)
            for (i = N - 1; i >= 0; i--)
            {
                routePlot[1][t] = p;
                routeI[1][t] = i;
                routeJ[1][t] = j;
                t++;
            }

    routeLen[1] = t;
}

void InitPlans() {
    int p, i, j, x = 1;
    for (p = 0; p < P; ++p) {
        for (i = 0; i < N; ++i) {
            for (j = 0; j < M; ++j) {
                plan[p][i][j] = x;
                x++;
                garden[p][i][j] = 0;
            }
        }
    }
}

DWORD WINAPI GardenerThread(PVOID pvParam) {
    int num = *(int*)pvParam;
    int cellIdx = 0;
    int p, i, j;
    int other = 1 - num;
    int mult;

    printf("gardener %d start\n", num + 1);

    if (num == 0) {
        mult = 10;
    }
    else {
        mult = 100;
    }

    curPlot[num] = -1;
    curI[num] = -1;
    curJ[num] = -1;

    while (cellIdx < routeLen[num]) {
        p = routePlot[num][cellIdx];
        i = routeI[num][cellIdx];
        j = routeJ[num][cellIdx];

        Lock(num);

        if (garden[p][i][j] != 0) { // обработано? дальше
            Unlock(num);
            cellIdx++;
            continue;
        }

        curPlot[num] = p;
        curI[num] = i;
        curJ[num] = j;

        if (curPlot[other] == p && curI[other] == i && curJ[other] == j) {
            if (p == 0 && i == 0 && j == 0) {
                continue;
            }
            printf("gardener %d got scared at plot %d cell [%d][%d]\n", num + 1, p + 1, i, j);

            curPlot[num] = -1;
            curI[num] = -1;
            curJ[num] = -1;

            Unlock(num);

            cellIdx -= 2;
            if (cellIdx < 0) {
                cellIdx = 0;
            }

            Sleep(100);
            continue;
        }

        garden[p][i][j] = plan[p][i][j] * mult;

        printf("gardener %d visited plot %d cell [%d][%d] -> %d\n", num + 1, p + 1, i, j, garden[p][i][j]);

        curPlot[num] = -1;
        curI[num] = -1;
        curJ[num] = -1;

        Unlock(num);

        cellIdx++;

        Sleep(30);
    }

    finished[num] = 1;

    printf("gardener %d finish\n", num + 1);

    return num;
}


int main(int argc, char** argv) {
    int x[2];
    DWORD dwThreadId[2];
    DWORD dw;
    DWORD dwResult_main[2];
    HANDLE hThread[2];

    InitPlans();
    BuildRoute1();
    BuildRoute2();

    finished[0] = 0;
    finished[1] = 0;

    printf("\nPLANS:\n");
    for (int p = 0; p < P; ++p) {
        printf("\nPlan of plot %d:\n", p + 1);
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < M; ++j) {
                printf("%4d", plan[p][i][j]);
            }
            printf("\n");
        }
    }

    for (int i = 0; i < 2; ++i) {
        x[i] = i;
        hThread[i] = CreateThread(NULL, 0, GardenerThread, (PVOID)&x[i], 0, &dwThreadId[i]);
        if (!hThread[i]) {
            printf("main process: thread %d not execute!\n", i);
        }
    }

    dw = WaitForMultipleObjects(2, hThread, TRUE, INFINITE);

    for (int i = 0; i < 2; ++i){
        GetExitCodeThread(hThread[i], &dwResult_main[i]);
        printf("gardener thread %d finished\n", (int)dwResult_main[i] + 1);
    }

    printf("\nRESULT GARDENS:\n");
    for (int p = 0; p < P; ++p) {
        printf("\nGarden plot %d:\n", p + 1);
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < M; ++j) {
                printf("%6d", garden[p][i][j]);
            }
            printf("\n");
        }
    }

    for (int i = 0; i < 2; ++i) {
        CloseHandle(hThread[i]);
    }

    return 0;
}