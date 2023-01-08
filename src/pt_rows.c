#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <omp.h>

#define SIZE 301
#define ALFA 1.0f
#define BETA 0.5f
#define GAMA 0.01f
#define NTHREADS 4

// celica
// s = kolicina vode
// type = tip celice: 0=zamrznjena, 1=mejna, 2=robna, 3=nedovzetna
// nx, ny = indexi sosedov celic

struct cell
{
    float s;
    int type;
    int nx[6];
    int ny[6];
};

struct cell ** field;
struct cell ** field2;
    
//da vemo, ali zapisujemo v file
bool progress = false;

//da vemo, ali smo na koncu
bool border = false;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void init_n(int i, int j)
{
    //inicializiraj indexe sosedov vsake celice
    field[i][j].nx[0] = i - 1;
    field[i][j].ny[0] = j;

    field[i][j].nx[1] = i;
    field[i][j].ny[1] = j - 1;

    field[i][j].nx[2] = i;
    field[i][j].ny[2] = j + 1;

    field[i][j].nx[3] = i + 1;
    field[i][j].ny[3] = j;

    if(j % 2 == 1)
    {
        field[i][j].nx[4] = i + 1;
        field[i][j].ny[4] = j - 1;

        field[i][j].nx[5] = i + 1;
        field[i][j].ny[5] = j + 1;
    }
    else
    {
        field[i][j].nx[4] = i - 1;
        field[i][j].ny[4] = j - 1;

        field[i][j].nx[5] = i - 1;
        field[i][j].ny[5] = j + 1;

    }
}

void printTab()
{
    for(int i = 0; i < SIZE; i++)
    {
        for(int j = 0; j < SIZE; j++)
        {
            printf("%d ", field[i][j].type);  
        }
        printf("\n");
    }
}

void printHex()
{
    for(int i = 0; i < SIZE; i++)
    {
        for(int j = 0; j < SIZE; j++)
        {
            if(j % 2 == 0 && field[i][j].type == 0)
                printf(" o ");
            else
                printf("   ");
        }
        printf("\n");
        for(int j = 0; j < SIZE; j++)
        {
            if(j % 2 == 1 && field[i][j].type == 0)
                printf(" o ");
            else
                printf("   ");
        }
        printf("\n");
    }
}

void *init_thread(void *arg)
{
    int *rank = (int *)arg;
    //printf("smo v threadu %d\n", *rank);
	int start = (int)(SIZE / (double)NTHREADS* *rank);
	int stop = (int)(SIZE / (double)NTHREADS*(*rank + 1));

    for(int i = start; i < stop; i++)
    {
        for(int j = 0; j < SIZE; j++)
        {
            field[i][j].s = BETA;
            if(field[i][j].type != 1)
                field[i][j].type = 3;
            init_n(i, j);
            if(i == 0 || j == 0 || i == SIZE - 1 || j == SIZE - 1)
            {
                //robna celica
                field[i][j].type = 2;
            }
            else if(i == SIZE / 2 && j == SIZE / 2)
            {
                //srednja celica
                field[i][j].s = 1.0;
                field[i][j].type = 0;

                for(int k = 0; k < 6; k++)
                {
                    pthread_mutex_lock(&lock);
                    //dovzetne celice
                    int x = field[i][j].nx[k];
                    int y = field[i][j].ny[k];
                    field[x][y].type = 1;
                    pthread_mutex_unlock(&lock);
                }

            }
        }
    }
}

void *step_thread(void *arg)
{
    int *rank = (int *)arg;
    //printf("smo v threadu %d\n", *rank);
	int start = (int)(SIZE / (double)NTHREADS* *rank);
	int stop = (int)(SIZE / (double)NTHREADS*(*rank + 1));

    for(int i = start; i < stop; i++)
    {
        for(int j = 0; j < SIZE; j++)
        {
            //izracunaj koliko vode dobi celica
            //robne celice imajo vedno BETA vode
            if(field[i][j].type != 0 && field[i][j].type != 2)
            {
                float sum = 0.0;
                for(int k = 0; k < 6; k++)
                {
                    // (potencialni) sosedi
                    int x = field[i][j].nx[k];
                    int y = field[i][j].ny[k];
                    if(x == -1 || y == -1 || x == SIZE || y == SIZE || field[x][y].type == 0 || field[x][y].type == 1)
                    {
                        //to ni indeks veljavnega soseda ALI pa gre za dovzetno celico, ne naredi nič
                        continue;
                    }
                    sum += field[x][y].s;
                }
                sum /= 6;
                if(field[i][j].type == 3)
                    sum -= field[i][j].s;
                sum *= (ALFA/2);
                if(field[i][j].type == 0 || field[i][j].type == 1)
                    sum += GAMA;
                
                field2[i][j].s += sum;
                if(field2[i][j].s >= 1 && field[i][j].type == 1)
                {
                    //imamo progress
                    progress = true;

                    //celica zmrzne
                    field2[i][j].type = 0;

                    //ce je zmrznila celica na robu polja, ne iteriramo vec
                    if(i == 1 || i == SIZE - 2 || j == 1 || j == SIZE - 2)
                    {
                        border = true;
                        continue;
                    }

                    //sosedi zamrznjene celice postanejo dovzetni
                    for(int k = 0; k < 6; k++)
                    {
                        int x = field[i][j].nx[k];
                        int y = field[i][j].ny[k];
                        if(x != -1 && y != -1 && x != SIZE && y != SIZE && field[x][y].type == 3)
                        {
                            pthread_mutex_lock(&lock);
                            field2[x][y].type = 1;
                            pthread_mutex_unlock(&lock);
                        }
                    }

                }
            } 
        }
    }

}

void *copy_array(void *arg)
{
    int *rank = (int *)arg;
	int start = (int)(SIZE / (double)NTHREADS* *rank);
	int stop = (int)(SIZE / (double)NTHREADS*(*rank + 1));
    for(int i = start; i < stop; i++)
    {
        memcpy(field2[i], field[i], SIZE * sizeof(struct cell));
    }
}

int main(int argc, char **argv)
{

    // ustvari tabelo
    field = (struct cell **)malloc(SIZE * sizeof(struct cell *));
    for(int i = 0; i < SIZE; i++)
        field[i] = (struct cell *)malloc(SIZE * sizeof(struct cell));

    //setaj up pthreads shit
    pthread_t t[NTHREADS];
    int p[NTHREADS];

    //merjenje casa
    double dt = omp_get_wtime();

    //inicializiraj vrednosti s threadi
    for (int i = 0; i<NTHREADS; i++)
    {
        p[i] = i;
		pthread_create(&t[i], NULL, init_thread, (void *)&p[i]);
    }
	for (int i = 0; i<NTHREADS; i++)
		pthread_join(t[i], NULL);

    //naredi kopijo tabele
    field2 = (struct cell **)malloc(SIZE * sizeof(struct cell *));
    for(int i = 0; i < SIZE; i++)
    {
        field2[i] = (struct cell *)malloc(SIZE * sizeof(struct cell));
        memcpy(field2[i], field[i], SIZE * sizeof(struct cell));
    }

    //setaj up text file
    char *filename = "pt_rows.txt";
    FILE *fp = fopen(filename, "w");
    if (fp == NULL)
    {
        printf("Error opening the file %s", filename);
        return -1;
    }

    //ponavljaj iteracije, dokler nisi na robu
    //v vsaki iteraciji se delo razdeli med niti
    while(!border)
    {
        for (int i = 0; i<NTHREADS; i++)
        {
            p[i] = i;
		    pthread_create(&t[i], NULL, step_thread, (void *)&p[i]);
        }
	    for (int i = 0; i<NTHREADS; i++)
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

        //obdelali smo vse celice, samo se kopiramo tabele
        struct cell **temp = field;
        field = field2;
        field2 = temp;
        for(int i = 0; i < NTHREADS; i++)
        {
            p[i] = i;
            pthread_create(&t[i], NULL, copy_array, (void *)&p[i]);
        }
        for (int i = 0; i<NTHREADS; i++)
		    pthread_join(t[i], NULL);
    }

    dt = omp_get_wtime() - dt;
    printf("time: %lf\n", dt);
    fclose(fp);
    free(field);
    free(field2);

    return 0;
}