#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <mpi.h>

#define POPULATION 200000
#define CURRENCIES 3
#define BANKS 3
#define MAX_MONEY 100000

long debts[BANKS][CURRENCIES];
long owed_to_me[BANKS][CURRENCIES];
long all_debts[BANKS][BANKS][CURRENCIES];

int random_bank(int current, int banks) {
    int r = rand() % (banks - 1);
    if (r >= current) r++;
    return r;
}

int main(int argc, char** argv) {
    int banks, current_bank;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &banks);
    MPI_Comm_rank(MPI_COMM_WORLD, &current_bank);

    if (banks != BANKS) {
        fprintf(stderr, "bank %d: banks must equal %d\n", current_bank, BANKS);
        MPI_Finalize();
        return 1;
    }

    srand((unsigned)(current_bank + 1) * 50);

    for (int i = 0; i < POPULATION; ++i) {
        bool wantsToWithdraw = (rand() % 2 == 0);
        int currency = rand() % CURRENCIES;
        long amount = rand() % MAX_MONEY + 1;

        if (wantsToWithdraw) {
            int destBank = random_bank(current_bank, banks);
            debts[destBank][currency] += amount;
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Allgather(
        debts, BANKS * CURRENCIES, MPI_LONG,
        all_debts, BANKS * CURRENCIES, MPI_LONG,
        MPI_COMM_WORLD
    );

    for (int j = 0; j < BANKS; ++j) {
        if (j == current_bank) {
            continue;
        }

        for (int c = 0; c < CURRENCIES; ++c) {
            owed_to_me[j][c] = all_debts[j][current_bank][c];
        }
    }

    for (int turn = 0; turn < banks; ++turn) {
        if (turn == current_bank) {
            printf("-----------Bank %d-----------\n", current_bank);
            for (int j = 0; j < banks; ++j) {
                if (j == current_bank) {
                    continue;
                }

                long net[CURRENCIES];
                for (int c = 0; c < CURRENCIES; ++c) {
                    net[c] = owed_to_me[j][c] - debts[j][c];
                }
                printf("\t on bank %d: net (CNY,USD,GBP) = %ld %ld %ld\n",
                    j, net[0], net[1], net[2]);
            }
            fflush(stdout);
        }
    }

    MPI_Finalize();
    return 0;
}