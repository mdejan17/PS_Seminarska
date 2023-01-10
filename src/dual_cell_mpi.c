#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <omp.h>

#include <mpi.h>

#define SIZE 60

#define ALFA 1.0f
#define BETA 0.5f
#define GAMA 0.01f

#define NTHREADS 1

#define MPI_INTERSECTION_WIDTH 2

struct cell
{
    float s[2];

    // 0=zamrznjena, 1=mejna, 2=robna, 3=nedovzetna
    char type[2];
    char changed;

    // padding or no?
    char padding[8]; // 12B + 4B = 16B -> 4 celli so 1 cache line (64 B)
};

struct cell *data;

struct cell **field;
struct cell **base_field;

int MPI_SIZE;
int MPI_RANK;

// abstract field len stats
int FIELD_START;
int FIELD_END;
int FIELD_LEN;

int MY_INTERSECTION_TOP;
int MY_INTERSECTION_BOT;

int steps = 0;

// da vemo, ali zapisujemo v file
bool progress = false;

// da vemo, ali smo na koncu
bool border = false;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t barrier;
// pthread_barrier_t barrier2;

void printHex(int f)
{
    for (int i = -MY_INTERSECTION_TOP; i < FIELD_LEN + MY_INTERSECTION_BOT; i++)
    {
        if (i == 0)
        {
            for (int j = 0; j < SIZE; j++)
            {
                printf(" - ");
            }
            printf("\n");
        }
        if (i == FIELD_LEN)
        {
            for (int j = 0; j < SIZE; j++)
            {
                printf(" - ");
            }
            printf("\n");
        }

        for (int j = 0; j < SIZE; j++)
        {
            if (j % 2 == 0)
            {
                if (field[i][j].type[f] == 0)
                    printf(" @ ");
                else if (field[i][j].type[f] == 1)
                    printf("   ");
                else if (field[i][j].type[f] == 2)
                    printf(" | ");
                else
                {
                    printf("   ");
                }
            }
            else
                printf("   ");
        }
        printf("\n");
        for (int j = 0; j < SIZE; j++)
        {
            if (j % 2 == 1)
            {
                if (field[i][j].type[f] == 0)
                    printf(" @ ");
                else if (field[i][j].type[f] == 1)
                    printf("   ");
                else if (field[i][j].type[f] == 2)
                    printf(" | ");
                else
                    printf("   ");
            }
            else
            {
                printf("   ");
            }
        }
        printf("\n");
    }
}

void printS(int f)
{
    for (int i = -MY_INTERSECTION_TOP; i < FIELD_LEN + MY_INTERSECTION_BOT; i++)
    {
        printf("%d ", MPI_RANK);
        if (i == 0)
        {
            for (int j = 0; j < SIZE; j++)
            {
                printf(" - ");
            }
            printf("\n");
            printf("%d ", MPI_RANK);
        }
        if (i == FIELD_LEN)
        {
            for (int j = 0; j < SIZE; j++)
            {
                printf(" - ");
            }
            printf("\n");
            printf("%d ", MPI_RANK);
        }
        for (int j = 0; j < SIZE; j++)
        {
            if (j % 2 == 0)
            {
                if (field[i][j].type[f] == 0)
                    printf("%.3f", field[i][j].s[f]);
                else
                {
                    printf("%.3f", field[i][j].s[f]);
                }
            }
            else
                printf("    ");
        }
        printf("\n");
        printf("%d ", MPI_RANK);
        for (int j = 0; j < SIZE; j++)
        {
            if (j % 2 == 1)
            {
                if (field[i][j].type[f] == 0)
                    printf("%.3f", field[i][j].s[f]);
                else
                {
                    printf("%.3f", field[i][j].s[f]);
                }
            }
            else
            {
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
    int start = (int)(FIELD_LEN / (double)NTHREADS * rank);
    int stop = (int)(FIELD_LEN / (double)NTHREADS * (rank + 1));

    if (rank == NTHREADS - 1)
    {
        stop = FIELD_LEN;
    }

    printf("[%d, %d)\n", start, stop);

    // if
    if (rank == 0 || rank == NTHREADS - 1)
    {
        if (MPI_RANK == 0 || MPI_RANK == MPI_SIZE - 1)
        {
            printf("top: %d bot: %d\n", MY_INTERSECTION_TOP, MY_INTERSECTION_BOT);
            stop += MY_INTERSECTION_BOT;
            start -= MY_INTERSECTION_TOP;
        }
    }
    printf("[%d, %d)\n", start, stop);

    for (int i = start; i < stop; i++)
    {
        // printf("rank %d i: %d\n", MPI_RANK, i);
        for (int j = 0; j < SIZE; j++)
        {

            field[i][j].changed = 1;
            field[i][j].s[0] = BETA;
            field[i][j].s[1] = BETA;
            if (field[i][j].type[0] != 1)
                field[i][j].type[0] = 3;
            if (field[i][j].type[1] != 1)
                field[i][j].type[1] = 3;
            // if u are mpi 0 and i 0 -> top border (robne celice)
            if (MPI_RANK == 0 && i == 0)
            {
                // robna celica
                field[i][j].type[0] = 2;
                field[i][j].type[1] = 2;

                // if u are last mpi and on last i -> bottom border (robne celice)
            }
            else if (MPI_RANK == MPI_SIZE - 1 && i == stop - 1)
            {
                field[i][j].type[0] = 2;
                field[i][j].type[1] = 2;
            }
            if (j == 0 || j == SIZE - 1) // sides (robne celice)
            {
                field[i][j].type[0] = 2;
                field[i][j].type[1] = 2;
            }
            // teoreticno bi lahko dau v pogoj še: MPI_RANK == MPI_SIZE/2 && ...
            else if ((FIELD_START + i) == SIZE / 2 && j == SIZE / 2)
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

                    field[x][y].type[0] = 1;
                }
            }
        }
    }
}

void *step_thread(void *arg)
{
    int rank = (long int)arg;

    int A = 0;
    int B = 1;
    int step = 0;
          
    int start = (int)(FIELD_LEN / (double)NTHREADS * rank);
    if (rank == 0)
    {
        start = 1 - MY_INTERSECTION_TOP;
    }
    int stop = (int)(FIELD_LEN / (double)NTHREADS * (rank + 1));
    if (rank == NTHREADS - 1)
    {
        stop = FIELD_LEN - 1 + MY_INTERSECTION_BOT;
    }

    //printf("STEP [%d, %d)\n", start, stop);
    // printf("smo v threadu %d : [%d, %d)\n", rank, start, stop);

    while (!border)
    {
        while (step < MPI_INTERSECTION_WIDTH)
        {
            int start_offset = 0;
            int end_offset = 0;

            if (rank == 0 && MY_INTERSECTION_TOP > 0)
            {
                start_offset = step;
            }

            if (rank == NTHREADS - 1 && MY_INTERSECTION_BOT > 0)
            {
                end_offset = step;
            }

            if (rank == 0 && MY_INTERSECTION_TOP > 0){
                for (int j = 0; j < SIZE; j++)
                {
                    field[start + start_offset-1][j].s[B] = field[start + start_offset-1][j].s[A];
                    field[start + start_offset-1][j].type[B] = field[start + start_offset-1][j].type[A];
                }
            }

            if(rank == NTHREADS - 1 && MY_INTERSECTION_BOT > 0){
                for (int j = 0; j < SIZE; j++)
                {
                    field[stop - end_offset][j].s[B] = field[stop - end_offset][j].s[A];
                    field[stop - end_offset][j].type[B] = field[stop - end_offset][j].type[A];
                }
            }

            

            //printf("r%d step: %d : [%d, %d)\n", MPI_RANK, step, start + start_offset, stop - end_offset);
            for (int i = start + start_offset; i < stop - end_offset; i++)
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
                        if (field[i][j].type[A] == 1)
                        {
                            field[i][j].type[B] = 1;
                            // printf("transitioned MEJNA (%d, %d)\n", i, j);
                        }
                        else
                        {
                            // printf(" > DOV (%d, %d)\n", i, j);
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

                            // prištej samo iz robnih in nedovzetnih (prostih) celic
                            if (field[x][y].type[A] == 2 || field[x][y].type[A] == 3)
                            {
                                sum += field[x][y].s[A];
                            }
                        }
                        sum /= 6;
                        if (field[i][j].type[A] == 3)
                            sum -= field[i][j].s[A];

                        sum *= (ALFA / 2);
                        if (field[i][j].type[A] == 1)
                            sum += GAMA;

                        sum += field[i][j].s[A];

                        /* if (i > 18)
                        {
                            if (j == 21)
                            {
                                printf("(%d, %d): read a field: %f\n", i, j, field[i][j].s[A]);
                                printf("(%d, %d): new sum: %f\n", i, j, sum);
                                printf("mid: %f\n", field[20][21].s[B]);
                            }
                        } */
                        field[i][j].s[B] = sum; // WRITING TO B
                        /* if ((i == 20) && j == 21)
                        {
                            printf("(%d, %d): written mid field: %f\n", i, j, field[20][j].s[B]);
                        } */
                        if (field[i][j].s[B] >= 1) // && field[i][j].type[B] == 1)
                        {
                            // imamo progress
                            progress = true;

                            // celica zmrzne
                            field[i][j].type[B] = 0; // WRITING TO B
                            field[i][j].changed = true;

                            // ce je zmrznila celica na robu polja, ne iteriramo vec
                            if (j == 1 || j == SIZE - 2)
                            {
                                border = true;
                                // continue;
                            }
                            if ((MPI_RANK == 0 && i == 1) || (MPI_RANK == MPI_SIZE - 1 && i == SIZE - 2))
                            {
                                border = true;
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
                                // if (x != -1 - MY_INTERSECTION_TOP && y != -1 && x != FIELD_LEN + MY_INTERSECTION_BOT && y != SIZE)
                                {
                                    if (field[x][y].type[B] == 3)
                                    {
                                        // pthread_mutex_lock(&lock);
                                        field[x][y].type[B] = 1; // WRITTING TO B
                                        // pthread_mutex_unlock(&lock);
                                    }
                                }
                            }
                        } // if it froze
                    }
                    else
                    { // zmrznjena ali robna... mem copy
                        // part of mem copy 2/2
                        if (field[i][j].changed)
                        {
                            // printf("transitioned %d ZMRZNENA (%d, %d)\n",rank , i, j);
                            field[i][j].type[B] = field[i][j].type[A];
                            field[i][j].s[B] = field[i][j].s[A];
                            field[i][j].changed = 0;
                        }
                    }
                } /// j
            }     // i

            A = (A * -1) + 1;
            B = (B * -1) + 1;

            //printf("(%d, %d): mid fieldA: %f\n", 20, 21, field[20][21].s[A]);

            /* printf("rank %d about to hop\n", rank);
            if(rank == 0){
                //printf("rank %d :\n", MPI_RANK);
                printHex(A);
                getchar();
            } else{
                printf("ignored rank 0");
            } */
            step++;
            // printf("waiting: %d\n", rank);
            pthread_barrier_wait(&barrier);
        } // while step
        void *buffer_ptr0;
        void *buffer_ptr1;

        void *recv_ptr0;
        void *recv_ptr1;
        int byte_length;

        MPI_Request req0;
        MPI_Request req1;

        //return NULL;
        if (rank == 0)
        {
            if (MPI_RANK == 0)
            {
                //printS(A);
                getchar();
            }
        }

        /* if(MPI_RANK == 1)
            printf("mid border s: %f\n", field[-1][20].s[B]); */

        // mpi transition
        if (rank == 0 && MPI_SIZE > 1)
        {
            if (MPI_RANK == 0)
            { // send to BOT
                //printf("0 transmiting..\n");
                buffer_ptr0 = (void *)&(field[FIELD_LEN - MPI_INTERSECTION_WIDTH][0]);
                //printf("0: trans i: %d\n", FIELD_LEN - MPI_INTERSECTION_WIDTH);
                recv_ptr0 = (void *)&(field[FIELD_LEN][0]);
                //printf("0: recv i: %d\n", FIELD_LEN);
                byte_length = SIZE * MY_INTERSECTION_BOT * sizeof(struct cell);

                //MPI_Isend(buffer_ptr0, byte_length, MPI_BYTE, MPI_RANK + 1, 0, MPI_COMM_WORLD, &req0); // imidiate send your data
                //printf("sent 0 -> 1\n");
                MPI_Recv(recv_ptr0, byte_length, MPI_BYTE, MPI_RANK + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // wait for your data
                //printf("recivd 0 <- 1\n");
                //MPI_Wait(&req0, MPI_STATUS_IGNORE); // wait till your data was delivered
                //printf("0 continue\n");
            }
            else if (MPI_RANK == MPI_SIZE - 1)
            { // send to TOP
                //printf("1 transmiting..\n");
                buffer_ptr0 = (void *)&(field[0][0]);
                //printf("1: trans i: %d\n", 0);
                recv_ptr0 = (void *)&(field[0 - MY_INTERSECTION_TOP][0]);
                //printf("1: recv i: %d\n", 0 - MY_INTERSECTION_TOP);
                byte_length = SIZE * MY_INTERSECTION_TOP * sizeof(struct cell);

                MPI_Isend(buffer_ptr0, byte_length, MPI_BYTE, MPI_RANK - 1, 0, MPI_COMM_WORLD, &req0); // imidiate send your data
                //printf("sent 1 -> 0\n");
                //MPI_Recv(recv_ptr0, byte_length, MPI_BYTE, MPI_RANK - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // wait for your data
                //printf("recivd 1 <- 0\n");
                MPI_Wait(&req0, MPI_STATUS_IGNORE); // wait till your data was delivered
                //printf("1 continue\n");
            }
            else
            { // send both ways
                // BOT DATA
                buffer_ptr0 = (void *)&(field[FIELD_LEN - MPI_INTERSECTION_WIDTH][0]);
                recv_ptr0 = (void *)&(field[FIELD_LEN][0]);
                byte_length = SIZE * MY_INTERSECTION_BOT * sizeof(struct cell);

                MPI_Isend(buffer_ptr0, byte_length, MPI_BYTE, MPI_RANK + 1, 0, MPI_COMM_WORLD, &req0);         // imidiate send your data
                MPI_Recv(recv_ptr0, byte_length, MPI_BYTE, MPI_RANK + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // wait for your data
                // dont wait for that yet, continue...

                // TOP DATA
                buffer_ptr1 = (void *)&(field[0][0]);
                recv_ptr1 = (void *)&(field[0 - MY_INTERSECTION_TOP][0]);
                byte_length = SIZE * MY_INTERSECTION_TOP * sizeof(struct cell);

                MPI_Isend(buffer_ptr1, byte_length, MPI_BYTE, MPI_RANK - 1, 0, MPI_COMM_WORLD, &req1);         // imidiate send your data
                MPI_Recv(recv_ptr1, byte_length, MPI_BYTE, MPI_RANK - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // wait for your data

                MPI_Wait(&req0, MPI_STATUS_IGNORE); // wait for BOT data delivery
                MPI_Wait(&req1, MPI_STATUS_IGNORE); // wait for TOP data delivery
            }
        }

        if (MPI_RANK == 0 && rank == 0)
        {
            pthread_barrier_wait(&barrier);
            printHex(A);
            getchar();
        }

        step = 0;
        pthread_barrier_wait(&barrier);

        // terminal print
        /* if(rank == NTHREADS -1 && progress){
            printHex(A);
        } */
    } // while border
    printf("rank: %d on BORDER!\n", MPI_RANK);

    // TODO mpi send stop algo signal
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &MPI_SIZE);
    MPI_Comm_rank(MPI_COMM_WORLD, &MPI_RANK);

    FIELD_START = (SIZE / MPI_SIZE) * MPI_RANK;
    FIELD_END = (SIZE / MPI_SIZE) * (MPI_RANK + 1);

    MY_INTERSECTION_BOT = MPI_INTERSECTION_WIDTH;
    MY_INTERSECTION_TOP = MPI_INTERSECTION_WIDTH;
    if (MPI_RANK == 0)
    { // first mpi has no intersection on top
        MY_INTERSECTION_TOP = 0;
    }
    if (MPI_RANK == MPI_SIZE - 1)
    { // last mpi has no intersection on bot
        MY_INTERSECTION_BOT = 0;
        FIELD_END = SIZE;
    }

    // printf("mpi rank: %d size: %d  [%d, %d)\n", MPI_RANK, MPI_SIZE, FIELD_START, FIELD_END);

    FIELD_LEN = FIELD_END - FIELD_START;

    // setup field, based on our intersections
    data = (struct cell *)malloc(SIZE * (FIELD_LEN + MY_INTERSECTION_BOT + MY_INTERSECTION_TOP) * sizeof(struct cell));
    base_field = (struct cell **)malloc((FIELD_LEN + MY_INTERSECTION_BOT + MY_INTERSECTION_TOP) * sizeof(struct cell *));
    for (int i = 0; i < FIELD_LEN + MY_INTERSECTION_BOT + MY_INTERSECTION_TOP; i++)
        base_field[i] = &(data[SIZE * i]);

    // field[-MY_INTERSECTION_TOP] = prvi element tabele

    field = &(base_field[MY_INTERSECTION_TOP]);

    // setaj up pthreads shit
    pthread_barrier_init(&barrier, NULL, NTHREADS);
    // pthread_barrier_init(&barrier2), NULL, NTHREADS;
    pthread_t t[NTHREADS];

    // setaj up text file
    char *filename = "pt_rows.txt";
    // char *filename = "bin/rezultati.txt";
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

    // printHex(0);
    // printf("b:\n");
    // printHex(B);
    // return 0;

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

    MPI_Finalize();

    fclose(fp);
    free(field);
    free(data);

    return 0;
}