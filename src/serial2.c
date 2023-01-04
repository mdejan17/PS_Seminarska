#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>

#define SIZE 301
#define ALFA 1.0f
#define BETA 0.5f
#define GAMA 0.01f

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

void init_n(struct cell **field, int i, int j)
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

void printTab(struct cell **field)
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

void printHex(struct cell **field)
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

bool end(struct cell **field)
{
    //ce so zamrznjene celice prisle do roba, koncamo
    for(int i = 1; i < SIZE - 1; i++)
    {
        if(field[i][1].type == 0 || field[i][SIZE - 1].type == 0)
            return true;
    }
    for(int j = 1; j < SIZE - 1; j++)
    {
        if(field[1][j].type == 0 || field[SIZE - 1][j].type == 0)
            return true;
    }
    return false;
}

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
    for(int i = 0; i < SIZE; i++)
        field[i] = (struct cell *)malloc(SIZE * sizeof(struct cell));

    //inicializiraj vrednosti

    for(int i = 0; i < SIZE; i++)
    {
        for(int j = 0; j < SIZE; j++)
        {
            field[i][j].s = BETA;
            if(field[i][j].type != 1)
                field[i][j].type = 3;
            init_n(field, i, j);
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
                    //dovzetne celice
                    int x = field[i][j].nx[k];
                    int y = field[i][j].ny[k];
                    field[x][y].type = 1;
                }

            }
        }
    }

    //naredi kopijo tabele
    struct cell **field2 = (struct cell **)malloc(SIZE * sizeof(struct cell *));
    for(int i = 0; i < SIZE; i++)
    {
        field2[i] = (struct cell *)malloc(SIZE * sizeof(struct cell));
        memcpy(field2[i], field[i], SIZE * sizeof(struct cell));
    }

    //setaj up text file
    char *filename = "rezultati.txt";
    FILE *fp = fopen(filename, "w");
    if (fp == NULL)
    {
        printf("Error opening the file %s", filename);
        return -1;
    }


    while(!end(field))
    {
        //ponavljaj iteracije, dokler nismo na robu

        for(int i = 0; i < SIZE; i++)
        {
            for(int j = 0; j < SIZE; j++)
            {
                //izracunaj koliko vode dobi celica
                //robne celice imajo vedno BETA vode
                if(field[i][j].type != 2)
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

                        //sosedi zamrznjene celice postanejo dovzetni
                        for(int k = 0; k < 6; k++)
                        {
                            int x = field[i][j].nx[k];
                            int y = field[i][j].ny[k];
                            if(x != -1 && y != -1 && x != SIZE && y != SIZE && field[x][y].type == 3)
                                field2[x][y].type = 1;
                        }
                    }
                } 
            }
        }

        //pisi v file
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
        }

        //obdelali smo vse celice, samo se kopiramo tabele
        struct cell **temp = field;
        field = field2;
        field2 = temp;
        for(int i = 0; i < SIZE; i++)
        {
            memcpy(field2[i], field[i], SIZE * sizeof(struct cell));
        }

    }
    endT = clock();
    cpu_time_used = ((double) (endT - start)) / CLOCKS_PER_SEC;
    printf("cajt: %f\n", cpu_time_used);
    fclose(fp);
    free(field);
    free(field2);

    return 0;
}