#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include "serial.h"

#define SIZE 100
#define ALFA 3.0f
#define BETA 0.7f
#define GAMA 0.01f
#define WRITE_FILE "rezultati.txt"

int main(int argc, char **argv)
{
    //merjenje casa
    clock_t start, endT;
    double cpu_time_used;
    start = clock();

    //da vemo, ali zapisujemo v file
    bool progress = false;

    // ustvari tabelo
    struct cell **field = (struct cell **)malloc(SIZE * sizeof(struct cell *));
    for(int i = 0; i < SIZE; i++) field[i] = (struct cell *)malloc(SIZE * sizeof(struct cell));

    //inicializiraj vrednosti
    init_table(field, SIZE, BETA);

    //naredi kopijo tabele
    struct cell **field2 = (struct cell **) malloc(SIZE * sizeof(struct cell *));
    for(int i = 0; i < SIZE; i++)
    {
        field2[i] = (struct cell *) malloc(SIZE * sizeof(struct cell));
        memcpy(field2[i], field[i], SIZE * sizeof(struct cell));
    }

    //setaj up text file
    FILE *fp = fopen(WRITE_FILE, "w");

    while(!end(field, SIZE)) // for every table 
    {
        //ponavljaj iteracije, dokler nismo na robu
        for(int i = 0; i < SIZE; i++)
            for(int j = 0; j < SIZE; j++)
                step(field, field2, &progress,  i,  j, SIZE, ALFA, GAMA);

        //pisi v file
        if(progress && !(progress = false))
            write_to_file(fp, field, SIZE);

        //obdelali smo vse celice, samo se kopiramo tabele
        struct cell **temp = field;
        field = field2, field2 = temp;
        for(int i = 0; i < SIZE; i++) memcpy(field2[i], field[i], SIZE * sizeof(struct cell));
    }

    //merjenje casa
    endT = clock();
    cpu_time_used = ((double) (endT - start)) / CLOCKS_PER_SEC;
    printf("Cas izvajanja: %fs\n", cpu_time_used);
    
    fclose(fp);
    free(field);
    free(field2);
    return 0;
}