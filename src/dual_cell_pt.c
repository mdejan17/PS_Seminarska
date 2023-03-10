#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <omp.h>

#define SIZE 40
#define ALFA 1.0f
#define BETA 0.5f
#define GAMA 0.01f
#define NTHREADS 4

struct cell
{
    float s[2];

    // 0=zamrznjena, 1=mejna, 2=robna, 3=nedovzetna
    char type[2];
    char changed;
    /* // field A cell
    float s_A;
    char type_A;

    // fied B cell
    float s_B;
    char type_B; */
};

struct cell *data;
struct cell **field;

//int A = 0;
//int B = 1;

// da vemo, ali zapisujemo v file
bool progress = false;

// da vemo, ali smo na koncu
bool border = false;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t barrier;
//pthread_barrier_t barrier2;

void printHex(int f)
{
    for (int i = 0; i < SIZE; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            if (j % 2 == 0) {
                if(field[i][j].type[f] == 0)
                    printf(" @ ");
                else if(field[i][j].type[f] == 1)
                    printf("   ");
                else if(field[i][j].type[f] == 2)
                    printf(" | ");
                else{
                    printf("   ");
                }
            } else
                printf("   ");
        }
        printf("\n");
        for (int j = 0; j < SIZE; j++)
        {
            if (j % 2 == 1) {
                if(field[i][j].type[f] == 0)
                    printf(" @ ");
                else if(field[i][j].type[f] == 1)
                    printf("   ");
                else if(field[i][j].type[f] == 2)
                    printf(" | ");
                else
                    printf("   ");
            } else{
                printf("   ");
            } 
        }
        printf("\n");
    }
}

void printS(int f)
{
    for (int i = 0; i < SIZE; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            if (j % 2 == 0) {
                if(field[i][j].type[f] == 0)
                    printf("%.3f", field[i][j].s[f]);
                else{
                    printf("%.3f", field[i][j].s[f]);
                }
            } else
                printf("    ");
        }
        printf("\n");
        for (int j = 0; j < SIZE; j++)
        {
            if (j % 2 == 1) {
                if(field[i][j].type[f] == 0)
                    printf("%.3f", field[i][j].s[f]);
                else{
                    printf("%.3f", field[i][j].s[f]);
                }
            } else{
                printf("    ");
            } 
        }
        printf("\n");
    }
}

void printTab()
{
    for (int i = 0; i < SIZE; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            printf("%d ", field[i][j].type[0]);
        }
        printf("\n");
    }
}

const int odd_neighbours[6][2] = {
    -1, 0,
    0, -1,
    0, +1,
    +1, 0,
    +1, -1,
    +1, +1};

const int even_neighbours[6][2] = {
    -1, 0,
    0, -1,
    0, +1,
    +1, 0,
    -1, -1,
    -1, +1};

void *init_thread(void *arg)
{
    int rank = (long int)arg;
    // printf("smo v threadu %d\n", *rank);
    int start = (int)(SIZE / (double)NTHREADS * rank);
    int stop = (int)(SIZE / (double)NTHREADS * (rank + 1));
    if(rank == NTHREADS-1){
        stop = SIZE;
    }

    for (int i = start; i < stop; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            field[i][j].changed = 1;
            field[i][j].s[0] = BETA;
            field[i][j].s[1] = BETA;
            if (field[i][j].type[0] != 1)
                field[i][j].type[0] = 3;
            if (field[i][j].type[1] != 1)
                field[i][j].type[1] = 3;
            // init_n(i, j);
            if (i == 0 || j == 0 || i == SIZE - 1 || j == SIZE - 1)
            {
                // robna celica
                field[i][j].type[0] = 2;
                field[i][j].type[1] = 2;
            }
            else if (i == SIZE / 2 && j == SIZE / 2)
            {
                // srednja celica
                field[i][j].s[0] = 1.0;
                field[i][j].type[0] = 0;

                int odd = j % 2;
                int x;
                int y;
                for (int k = 0; k < 6; k++)
                {
                    // dovzetne celice
                    if (odd)
                    {
                        x = i + odd_neighbours[k][0];
                        y = j + odd_neighbours[k][1];
                    }
                    else
                    {
                        x = i + even_neighbours[k][0];
                        y = j + even_neighbours[k][1];
                    }
                    //pthread_mutex_lock(&lock);
                    field[x][y].type[0] = 1;
                    //pthread_mutex_unlock(&lock);
                }
            }
        }
    }
}

// swaps field pointers and handles mem copy
void hop()
{
    //A = (A * -1) + 1;
    //B = (B * -1) + 1;
}

void *step_thread(void *arg)
{
    int A = 0;
    int B = 1;
    
    int rank = (long int)arg;
    int start = (int)(SIZE / (double)NTHREADS * rank);
    if (rank == 0)
    {
        start = 1;
    }
    int stop = (int)(SIZE / (double)NTHREADS * (rank + 1));
    if (rank == NTHREADS - 1)
    {
        stop = SIZE - 1;
    }
    //printf("smo v threadu %d : [%d, %d)\n", rank, start, stop);

    while (!border)
    {
        for (int i = start; i < stop; i++)
        {
            for (int j = 1; j < SIZE - 1; j++)
            {

                int odd = j % 2;
                int x;
                int y;
                // izracunaj koliko vode dobi celica
                // robne celice imajo vedno BETA vode
                if (field[i][j].type[A] != 0 && field[i][j].type[A] != 2)
                {
                    // part of mem_copy 1/2
                    if(field[i][j].type[A] == 1){
                        field[i][j].type[B] = 1;
                        //printf("transitioned MEJNA (%d, %d)\n", i, j);
                    } else{
                        //printf(" > DOV (%d, %d)\n", i, j);
                    }
                    float sum = 0;
                    for (int k = 0; k < 6; k++)
                    {
                        // (potencialni) sosedi
                        if (odd)
                        {
                            x = i + odd_neighbours[k][0];
                            y = j + odd_neighbours[k][1];
                        }
                        else
                        {
                            x = i + even_neighbours[k][0];
                            y = j + even_neighbours[k][1];
                        }

                        // pri??tej samo iz robnih in nedovzetnih (prostih) celic
                        if (field[x][y].type[A] == 2 || field[x][y].type[A] == 3)
                        {
                            sum += field[x][y].s[A];
                        }
                    }
                    sum /= 6;
                    if(field[i][j].type[A] == 3)
                        sum -= field[i][j].s[A];

                    sum *= (ALFA / 2);
                    if (field[i][j].type[A] == 1)
                        sum += GAMA;

                    sum += field[i][j].s[A];

                    field[i][j].s[B] = sum; // WRITING TO B
                    if (field[i][j].s[B] >= 1) // && field[i][j].type[B] == 1)
                    {
                        // imamo progress
                        progress = true;

                        // celica zmrzne
                        field[i][j].type[B] = 0; // WRITING TO B
                        field[i][j].changed = true;

                        // ce je zmrznila celica na robu polja, ne iteriramo vec
                        if (i == 1 || i == SIZE - 2 || j == 1 || j == SIZE - 2)
                        {
                            border = true;
                            //continue;
                        }

                        // sosedi zamrznjene celice postanejo dovzetni
                        for (int k = 0; k < 6; k++)
                        {
                            // (potencialni) sosedi
                            if (odd)
                            {
                                x = i + odd_neighbours[k][0];
                                y = j + odd_neighbours[k][1];
                            }
                            else
                            {
                                x = i + even_neighbours[k][0];
                                y = j + even_neighbours[k][1];
                            }
                            if (x != -1 && y != -1 && x != SIZE && y != SIZE)
                            {
                                if(field[x][y].type[B] == 3){
                                    //pthread_mutex_lock(&lock);
                                    field[x][y].type[B] = 1; // WRITTING TO B
                                    //pthread_mutex_unlock(&lock);
                                }
                            }
                        }
                    } // if it froze
                } else{ // zmrznjena ali robna... mem copy
                    // part of mem copy 2/2
                    if(field[i][j].changed){
                        //printf("transitioned %d ZMRZNENA (%d, %d)\n",rank , i, j);
                        field[i][j].type[B] = field[i][j].type[A];
                        field[i][j].s[B] = field[i][j].s[A];
                        field[i][j].changed = 0;
                    }
                }
            } /// j
        } // i

        //pthread_barrier_wait(&barrier);
        A = (A * -1) + 1;
        B = (B * -1) + 1;
        pthread_barrier_wait(&barrier);

        // terminal print
        /* if(rank == NTHREADS -1 && progress){
            printHex(A);
        } */
    } // step loop
}

int main(int argc, char **argv)
{
    printf("Cell size: %ldB\n", sizeof(struct cell));
    // ustvari tabelo
    data = (struct cell *)malloc(SIZE * SIZE * sizeof(struct cell));
    field = (struct cell **)malloc(SIZE * sizeof(struct cell*));
    for (int i = 0; i < SIZE; i++)
        field[i] = &(data[SIZE * i]);

    // setaj up pthreads shit
    pthread_barrier_init(&barrier, NULL, NTHREADS);
    //pthread_barrier_init(&barrier2), NULL, NTHREADS;
    pthread_t t[NTHREADS];

    // setaj up text file
    char *filename = "pt_rows.txt";
    //char *filename = "bin/rezultati.txt";
    FILE *fp = fopen(filename, "w");
    if (fp == NULL)
    {
        printf("Error opening the file %s", filename);
        return -1;
    }

    // merjenje casa
    double dt = omp_get_wtime();

    // inicializiraj vrednosti s threadi
    for (int i = 0; i < NTHREADS; i++)
    {
        pthread_create(&t[i], NULL, init_thread, (void *)(long int)i);
    }
    for (int i = 0; i < NTHREADS; i++)
        pthread_join(t[i], NULL);

    printHex(0);

    for (int i = 0; i < NTHREADS; i++)
    {
        pthread_create(&t[i], NULL, step_thread, (void *)(long int)i);
    }
    for (int i = 0; i < NTHREADS; i++)
        pthread_join(t[i], NULL);

    /*//pisi v file
    if(progress)
    {
        progress = false;
        for (int i = 0; i < SIZE; i++)
        {
            for (int j = 0; j < SIZE; j++)
            {
                fprintf(fp, "%d ", field[i][j].type);
            }
            fprintf(fp, "\n");
        }
        fprintf(fp, "\n");
    }*/

    dt = omp_get_wtime() - dt;
    printf("time: %lf\n", dt);
    fclose(fp);
    free(field);
    free(data);

    return 0;
}