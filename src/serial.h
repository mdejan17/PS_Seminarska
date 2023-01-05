#include <stdio.h>

struct cell
{
    float s;   // kolicina vode
    int type;  // 0=zamrznjena, 1=mejna, 2=robna, 3=nedovzetna
    int nx[6];
    int ny[6]; // indexi sosedov celic
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
    field[i][j].ny[4] = j - 1;
    field[i][j].ny[5] = j + 1;
    field[i][j].nx[4] = (j % 2 == 1) ? i + 1 : i - 1;
    field[i][j].nx[5]  = (j % 2 == 1) ? i + 1 : i - 1;
}

void printTab(struct cell **field, int size)
{
    for(int i = 0; i < size; i++)
    {
        for(int j = 0; j < size; j++)
            printf("%d ", field[i][j].type);  
        printf("\n");
    }
}

void printHex(struct cell **field, int size)
{
    for(int i = 0; i < size; i++)
    {
        for(int j = 0; j < size; j++)
        {
            if(j % 2 == 0 && field[i][j].type == 0)
                printf(" o ");
            else
                printf("   ");
        }
        printf("\n");
        for(int j = 0; j < size; j++)
        {
            if(j % 2 == 1 && field[i][j].type == 0)
                printf(" o ");
            else
                printf("   ");
        }
        printf("\n");
    }
}

bool end(struct cell **field, int size)
{
    //ce so zamrznjene celice prisle do roba, koncamo
    for(int i = 1; i < size - 1; i++)
        if(field[i][1].type == 0 || field[i][size - 2].type == 0) return true;
    for(int j = 1; j < size - 1; j++)
        if(field[1][j].type == 0 || field[size - 2][j].type == 0) return true;
    return false;
}