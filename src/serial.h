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

void init_table(struct cell **field, int size, float beta)
{
    for(int i = 0; i < size; i++)
    {
        for(int j = 0; j < size; j++)
        {
            field[i][j].s = beta;
            if(field[i][j].type != 1)
                field[i][j].type = 3; //byfrost 
            init_n(field, i, j);
            if(i == 0 || j == 0 || i == size - 1 || j == size - 1)
                field[i][j].type = 2; // robna celica
            else if(i == size / 2 && j == size / 2)
            {
                //srednja celica
                field[i][j].s = 1.0;
                field[i][j].type = 0;
                for(int k = 0; k < 6; k++)
                {
                    //mejne
                    int x = field[i][j].nx[k];
                    int y = field[i][j].ny[k];
                    field[x][y].type = 1; 
                }
            }
        }
    }
}

void step(struct cell **field, struct cell **field2, bool *progress, int i, int j, int size, float alfa, float gama)
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
            if(!(x == -1 || y == -1 || x == size || y == size || field[x][y].type == 0 || field[x][y].type == 1))//to ni indeks veljavnega soseda ALI pa gre za dovzetno celico, ne naredi nič
                sum += field[x][y].s;
        }
        sum /= 6;
        if(field[i][j].type == 3)
            sum -= field[i][j].s;
        sum *= (alfa/2);
        if(field[i][j].type == 0 || field[i][j].type == 1)
            sum += gama;
        field2[i][j].s += sum;
        if(field2[i][j].s >= 1 && field[i][j].type == 1)
        {
            //imamo progress
            *progress = true;
            //celica zmrzne
            field2[i][j].type = 0;
            //sosedi zamrznjene celice postanejo dovzetni
            for(int k = 0; k < 6; k++)
            {
                int x = field[i][j].nx[k];
                int y = field[i][j].ny[k];
                if(x != -1 && y != -1 && x != size && y != size && field[x][y].type == 3)
                    field2[x][y].type = 1;
            }
        }
    } 
}



void write_to_file(FILE *fp, struct cell **field, int size)
{
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
            fprintf(fp, "%d ", field[i][j].type);
        fprintf(fp, "\n");
    }
    fprintf(fp, "\n");

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

// [DEBUG] not used
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