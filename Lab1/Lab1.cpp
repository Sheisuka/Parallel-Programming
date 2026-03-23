#include <stdio.h>
#include <conio.h>
#include <windows.h>

const int N = 100;
const int K = 32;

int n, k;
int a[N][N];

int found[K];
int resI[K];
int resJ[K];

DWORD WINAPI ThreadFunc(PVOID pvParam){
    int num, i, j;

    num = *(int*)pvParam;

    printf("thread %d started\n", num);

    found[num] = 0;
    resI[num] = -1;
    resJ[num] = -1;

    for (i = 0; i < n; ++i) {
        if (!(i % k == num || i % k == (num - 1 + k) % k)) {
            continue;
        }
        for (j = 0; j < n; j++) {
            if (a[i][j] == 1) {
                found[num] = 1;
                resI[num] = i;
                resJ[num] = j;
            }
        }
    }

    printf("thread %d finished\n", num);

    return DWORD(num);
}

// Код основного потока
int main(int argc, char** argv)
{
    int x[K];
    DWORD dwThreadId[K];
    DWORD dw;
    DWORD dwResult_main[K];
    HANDLE hThread[K];

    int i, j;
    int shipFound = 0;
    int shipI = -1;
    int shipJ = -1;

    printf("number on rows n: ");
    scanf_s("%d", &n);

    printf("number of threads k: ");
    scanf_s("%d", &k);


    printf("the ship location i j:\n");
    scanf_s("%d %d", &i, &j);
    if ((i < 0 || i > n) || (j < 0 || j > n)) {
        printf("invalid position\n");
        return -1;
    }
    a[i][j] = 1;

    for (i = 0; i < k; ++i) {
        x[i] = i;

        hThread[i] = CreateThread(NULL, 0, ThreadFunc, (PVOID)&x[i], 0, &dwThreadId[i]);

        if (!hThread[i]) {
            printf("main: thread %d not execute!\n", i);
        }
    }

    dw = WaitForMultipleObjects(k, hThread, TRUE, INFINITE);

    for (i = 0; i < k; ++i) {
        GetExitCodeThread(hThread[i], &dwResult_main[i]);
        printf("thread %d finished\n", (int)dwResult_main[i]);
    }

    for (i = 0; i < k; ++i) {
        if (found[i] == 1) {
            shipFound = 1;
            shipI = resI[i];
            shipJ = resJ[i];
            break;
        }
    }

    if (shipFound == 1) {
        printf("\nthe ship was fount at: [%d][%d]\n", shipI, shipJ);
    }
    else {
        printf("\nthe ship was not found\n");
    }

    for (i = 0; i < k; ++i) {
        CloseHandle(hThread[i]);
    }

    return 0;
}