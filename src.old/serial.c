#include <stdio.h>
#include <stdlib.h>

#include "draw_hexgrid_offsetarray.h"

#define STEPS 500
//#define FIELD_LEN 25
//#define FIELD_LEN 60
#define FIELD_LEN 1000

#define ALFA 1.0
#define BETA 0.4
#define GAMA 1.0f


// defines which raw is moved to right (completely depends on how we'd like to represent hex field)
#define START_OFFSET 0



// int write_hex_to_file(char *filename);
// int get_grey_scale_array(char *grey_scale_arr, int hex_w, int starting_field_offset);
// void draw_grey_hex(char *start);

void init_field_data_structure(int len);
void init_field(double beta);
void print_s_data(cell **field);
void print_flag_data(cell **field);
void print_flag_data_limit(cell **field, int offset);
void print_frozen_data(cell **field);

int get_local_offset(int row);

void handle_cell(int i, int j);
void handle_transition();
void add_transition(int i, int j);
void by_freeze_neighbours(int i, int j, cell **write_to_field);
void calc_cell(int i, int j);
void calc_frozen(int i, int j);
void calc_free(int i, int j);
void fetch_neighbours(double *addition, int i, int j);

void step();

void set_frozen();

struct cell data_field_A[FIELD_LEN * FIELD_LEN];
struct cell data_field_B[FIELD_LEN * FIELD_LEN];

cell *field_A[FIELD_LEN];
cell *field_B[FIELD_LEN];

// pointer values get swaped on iterations (might get thread local values for multithread)
cell **curr_field;
cell **new_field;

int frozen_cnter = 1;

freeze_transition transition = {
    .i = new int[(FIELD_LEN-2) * (FIELD_LEN-2)],
    .j = new int[(FIELD_LEN-2) * (FIELD_LEN-2)],
    .len = 0,
};

int main(int argc, char **argv){
    init_field_data_structure(FIELD_LEN);
    init_field(BETA);
    //set_frozen();
    //print_flag_data(curr_field);
    //print_frozen_data(curr_field);
    //print_s_data(curr_field);
    //show_on_screen(15, FIELD_LEN, curr_field, frozen_cnter);
    //getc(stdin);

    for(int i = 0; i < STEPS; i++){
        if(i % 100){
            //printf("-(%d)-----\n", (i-1));
            //print_flag_data_limit(curr_field, (FIELD_LEN/2 - 20));
            
            //show_on_screen(10, FIELD_LEN, curr_field, frozen_cnter);

        }
        //printf("LALA");
        //printf("-(%d)-----\n", (i-1));
        //print_s_data(curr_field);
        //print_frozen_data(curr_field);
        //print_flag_data(curr_field);
        //printf("frozen_cnter: %d\n", frozen_cnter);

        //getc(stdin);
        step();
    }
    printf("--------------------final state:------------------\n");

    //write_hex_to_file("hello_world.jpg");

    show_on_screen(10, FIELD_LEN, curr_field, frozen_cnter);

    //print_s_data(curr_field);
    //print_flag_data(curr_field);

}

void set_frozen(){
    for(int i = 1; i < FIELD_LEN-1; i++){
        for(int j = 1; j < FIELD_LEN-1; j++){
            //printf("");
            curr_field[i][j].flag = 0;
            curr_field[i][j].s = 3.f;
            frozen_cnter++;
        }
    }
}

void step(){
    cell **temp;
    //printf("flag data to be used\n");
    //print_flag_data(curr_field);

    if(transition.len > 0){
        //printf("handling transiiton\n");
        //print_flag_data(new_field);
        handle_transition();
        //printf("after handling: \n");
        //print_flag_data(new_field);
        //getc(stdin);
        //printf("--\n");
    }

    for(int i = 1; i < FIELD_LEN-1; i++){
        for (int j = 1; j < FIELD_LEN-1; j++){
            // calc:
            handle_cell(i, j);
        }
    }



    // switch fields:
    temp = curr_field;
    curr_field = new_field;
    new_field = temp;

    //printf("step\n");
}

// processing
void handle_cell(int i, int j){
    
    // if cell IS NOT frozen calculate
    if(curr_field[i][j].s < 1){
        calc_cell(i, j);
        if(new_field[i][j].s >= 1){
            //printf("freezin boi (%d, %d)...\n", i, j);
            //print_flag_data(curr_field);
            by_freeze_neighbours(i, j, new_field);
            frozen_cnter++;
            add_transition(i, j);
            
            //getc(stdin);
        }
    } else{ // if cell IS frozen, there's no need to calculate it
        new_field[i][j] = curr_field[i][j];
    }

    //
}

// transitions new by_frozen_neighbours to new_field
void handle_transition(){
    //printf("in transiton...\n");
    while(transition.len > 0){
        transition.len--;
        //printf("len: %d\n", transition.len);
        by_freeze_neighbours(transition.i[transition.len], transition.j[transition.len], new_field);
    }
}

void add_transition(int i, int j){
    transition.i[transition.len] = i;
    transition.j[transition.len] = j;

    transition.len++;
}

// by freeze neighbours of given cell, writing to specified field
void by_freeze_neighbours(int i, int j, cell **write_to_field){
    int local_offset = get_local_offset(i);
    // left right
    if(write_to_field[i][j-1].flag == 2){
        write_to_field[i][j-1].flag = 0;
    }
    if(write_to_field[i][j+1].flag == 2){
        write_to_field[i][j+1].flag = 0;
    }

    // top l r

    if(write_to_field[i-1][j - local_offset].flag == 2){
        write_to_field[i-1][j - local_offset].flag = 0;
    }
    if(write_to_field[i-1][j+1 - local_offset].flag == 2){
        write_to_field[i-1][j+1 - local_offset].flag = 0;
    }

    // bot l r

    if(write_to_field[i+1][j - local_offset].flag == 2){
        write_to_field[i+1][j - local_offset].flag = 0;
    }
    if(write_to_field[i+1][j+1 - local_offset].flag == 2){
        write_to_field[i+1][j+1 - local_offset].flag = 0;
    }
}

// computing funcs

void calc_cell(int i, int j){
    //printf("-----calcing cell-----\n");
    //printf("cell flag: %d\n", (int)curr_field[i][j].flag);
    if(curr_field[i][j].flag == 0){
        calc_frozen(i, j);
    } else if(curr_field[i][j].flag == 2){
        calc_free(i, j);
    } else { // this is 100% edge cell :)
        // handle edge cell:

        // should not need handling, as it should never be overwritten
        //new_field[i][j].s = BETA;
    }
    ////printf("new_val");
    //printf("-----calced-------\n");
}

// water addition for frozen cells (actually by_frozen cells)
void calc_frozen(int i, int j){
    //printf("calc_frozen\n");
    // kolicina, ki se bo dodala trenutnjemu stanju vode v celici
    //printf("(%d, %d)\n", i, j);
    double addition = 0;
    fetch_neighbours(&addition, i, j);
    //printf("raw addition: %lf\n", addition);
    addition /= 6;
    addition = addition * (ALFA / 2);
    
    //printf("adding gamma(%lf) to %lf\n", GAMA, curr_field[i][j].s + addition);
    addition += GAMA;
    

    //printf("actual addition: %lf\n", addition);
    //getc(stdin);
    
    new_field[i][j].s = curr_field[i][j].s + addition;
}

// water addition for free cells
void calc_free(int i, int j){
    //printf("calc_free\n");
    // kolicina, ki se bo dodala trenutnjemu stanju vode v celici
    double addition = 0;
    fetch_neighbours(&addition, i, j);
    //printf("raw addition: %lf\n", addition);
    addition /= 6;
    addition -= curr_field[i][j].s;
    addition = addition * (ALFA / 2);
    //printf("actual addition: %lf\n", addition);
    
    new_field[i][j].s = curr_field[i][j].s + addition;
}

// get raw amount of fetchable water from neighbours
void fetch_neighbours(double *addition, int i, int j){
    //printf("fetching neighbour: (%d, %d)\n", i, j);
    int local_offset = get_local_offset(i);

    // left right
    if(curr_field[i][j-1].flag != 0){
        //printf("fetching left one: %lf\n", curr_field[i][j-1].s);
        *addition += curr_field[i][j-1].s;
    }
    if(curr_field[i][j+1].flag != 0){
        *addition += curr_field[i][j+1].s;
    }

    //printf("left_rifht_add: %lf\n", *addition);

    // top l r

    if(curr_field[i-1][j - local_offset].flag != 0){
        *addition += curr_field[i-1][j - local_offset].s;
    }
    if(curr_field[i-1][j+1 - local_offset].flag != 0){
        *addition += curr_field[i-1][j+1 - local_offset].s;
    }

    //printf("top_l_r_add: %lf\n", *addition);

    // bot l r

    if(curr_field[i+1][j - local_offset].flag != 0){
        *addition += curr_field[i+1][j - local_offset].s;
    }
    if(curr_field[i+1][j+1 - local_offset].flag != 0){
        *addition += curr_field[i+1][j+1 - local_offset].s;
    }
}

// cell stuff

int get_local_offset(int row){
    return (row + START_OFFSET) % 2;
}


// initialisation and helper functions:

void init_field_data_structure(int len){
    for(int i = 0; i < len; i++){
        field_A[i] = data_field_A + (i * len);
        field_B[i] = data_field_B + (i * len);
    }

    curr_field = field_A;
    new_field = field_B;
}

void init_field(double beta){

    // set whole field with BETA and free flag
    for(int i = 1; i < FIELD_LEN-1; i++){
        for(int j = 1; j < FIELD_LEN-1; j++){
            curr_field[i][j].s = beta;
            curr_field[i][j].flag = 2;
            //printf("%c");

            new_field[i][j].flag = 2;
        }
    }

    // set edges to BETA and edge flag (for both fields)
    for(int i = 0; i < FIELD_LEN; i++){
        // esge flag set
        {
            new_field[i][0].flag = 3;
            new_field[i][0].flag = 3;

            curr_field[i][0].flag = 3;
            curr_field[i][0].flag = 3;

            new_field[i][FIELD_LEN-1].flag = 3;
            new_field[i][FIELD_LEN-1].flag = 3;

            curr_field[i][FIELD_LEN-1].flag = 3;
            curr_field[i][FIELD_LEN-1].flag = 3;

            new_field[0][i].flag = 3;
            new_field[0][i].flag = 3;

            curr_field[0][i].flag = 3;
            curr_field[0][i].flag = 3;

            new_field[FIELD_LEN-1][i].flag = 3;
            new_field[FIELD_LEN-1][i].flag = 3;

            curr_field[FIELD_LEN-1][i].flag = 3;
            curr_field[FIELD_LEN-1][i].flag = 3;
        }
        //edge water set
        {
            new_field[i][0].s = BETA;
            new_field[i][0].s = BETA;

            curr_field[i][0].s = BETA;
            curr_field[i][0].s = BETA;

            new_field[i][FIELD_LEN-1].s = BETA;
            new_field[i][FIELD_LEN-1].s = BETA;

            curr_field[i][FIELD_LEN-1].s = BETA;
            curr_field[i][FIELD_LEN-1].s = BETA;

            new_field[0][i].s = BETA;
            new_field[0][i].s = BETA;

            curr_field[0][i].s = BETA;
            curr_field[0][i].s = BETA;

            new_field[FIELD_LEN-1][i].s = BETA;
            new_field[FIELD_LEN-1][i].s = BETA;

            curr_field[FIELD_LEN-1][i].s = BETA;
            curr_field[FIELD_LEN-1][i].s = BETA;
        }
    }

    int mid = FIELD_LEN/2;

    // freeze mid cell and by_freze around it
    int local_offset = get_local_offset(mid);

    // mid cell set to frozen
    curr_field[mid][mid].s = 1;
    curr_field[mid][mid].flag = 0;

    //new_field[mid][mid].s = 1;
    new_field[mid][mid].flag = 0;

    // cset ells around mid to by_frozen
    by_freeze_neighbours(mid, mid, curr_field);
    add_transition(mid, mid);
}

void print_s_data(cell **field){
    for(int i = 0; i < FIELD_LEN; i++){
        if(i % 2 == START_OFFSET)
        {
            printf("   ");
        }
        for(int j = 0; j < FIELD_LEN; j++){
            printf("(%.2lf) ", field[i][j].s);
        }
        printf("\n");
    }
}

void print_flag_data(cell **field){
    for(int i = 0; i < FIELD_LEN; i++){
        if(i % 2 == START_OFFSET)
        {
            printf("   ");
        }
        for(int j = 0; j < FIELD_LEN; j++){
            if(field[i][j].s >= 1){
                printf("[X%dX] ", (int)field[i][j].flag);
            } else{
                printf("(_%d_) ", (int)field[i][j].flag);
            }
        }
        printf("\n");
    }
}

void print_flag_data_limit(cell **field, int offset){
    int limit = 42;
    for(int i = 0; i < limit; i++){
        if(i % 2 == START_OFFSET)
        {
            printf("   ");
        }
        for(int j = 0; j < limit; j++){
            if(field[offset + i][offset + j].s >= 1){
                printf("[X%dX] ", (int)field[offset + i][offset + j].flag);
            } else{
                printf("(_%d_) ", (int)field[offset + i][offset + j].flag);
            }
        }
        printf("\n");
    }
}

void print_frozen_data(cell **field){
    for(int i = 0; i < FIELD_LEN; i++){
        if(i % 2 == START_OFFSET)
        {
            printf("   ");
        }
        for(int j = 0; j < FIELD_LEN; j++){
            if(field[i][j].s >= 1){
                printf("( O ) ");
            }else{
                printf("(   ) ");
            }
        }
        printf("\n");
    }
}

// draw data stuff:
