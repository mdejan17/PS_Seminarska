#include "draw_hexgrid_offsetarray.h"

void write_to_diskimg(int hex_size, int arr_size, cell **hex_grid, int frozen_cells_cnt){

}

void show_on_screen(const int hex_size, const int arr_size, cell **hex_grid, const int frozen_cells_cnt){
    printf("first thing in show_on_screen...\n");
    double d_size = (double) hex_size;
    printf("allocating shapes array... \n");
    printf("frozen_cells_cnt: %d\n", frozen_cells_cnt);
    //sf::CircleShape *shapes = malloc(sizeof(sf::CircleShape) * frozen_cells_cnt);
    //sf::CircleShape shapes[frozen_cells_cnt];
    printf("done!\n");

    calc_draw(hex_grid, arr_size, hex_size);
    printf("drawn\n");

    /* if(shape_hexgrid(shapes, d_size, arr_size, hex_grid) != frozen_cells_cnt){
        printf("Wrong amount of forzen cells.\n");
    } */
    //printf("shaped_hexgrid -> going to draw...\n");
    //draw_on_screen(shapes, frozen_cells_cnt, hex_size * arr_size * 2);
    //free(shapes);
}

int shape_hexgrid(sf::CircleShape *shape_arr, const double hex_size, const int arr_size, cell **hex_grid){
    printf("shaping hexgrid...\n");
    int cnt = 0;
    for(int i = 0; i < arr_size; i++){
        for(int j = 0; j < arr_size; j++){
            if(hex_grid[i][j].s >= 1){
                set_shape_hexcell((shape_arr + cnt), hex_size, i, j);
                cnt++;
            }
        }
    }
    printf("cnt size: %d\n", cnt);
    return cnt;
}

//static const sf::Texture hex_shape = sh;

void set_shape_hexcell(sf::CircleShape *shape, const double size, int i, int j){
    // printf("hex_cell: (%d, %d)\n", i, j);

    const int basic_offset = size;

    double offset_x;
    double offset_y;

    evenq_offset_to_pixel(&offset_x, &offset_y, size, i, j);
    
    //sf::CircleShape shape(size, 6);
    (*shape) = sf::CircleShape(size-2, 6);

    (*shape).setOrigin(size, size);
    (*shape).setPosition(basic_offset + offset_x, basic_offset + offset_y);

    // outline
    (*shape).setOutlineThickness(1);
    (*shape).setOutlineColor(sf::Color(250, 150, 100));

    //(*shape).setRotation(90.f);
    (*shape).setFillColor(sf::Color::Green);

}

void draw_on_screen(sf::CircleShape *arr, const int len, int window_size){
    sf::RenderWindow window(sf::VideoMode(window_size, window_size), "grid");
    printf("drawing on screen...\n");
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }
        window.clear({60, 60, 60, 255});
        for(int i = 0; i < len; i++){
            window.draw(arr[i]);
        }

        window.display();
    }

    return ;
}

void calc_draw(cell **hex_grid, int arr_size, double hex_size){

    // printf("about to calc_draw...\n");
    sf::CircleShape shape;
    //int cnt = 0;

    // printf("calc_drawing...\n");

    sf::RenderWindow window(sf::VideoMode(hex_size *arr_size *2, hex_size *arr_size *2), "grid");
    // let's define a view
    int tomid_offset = (hex_size *arr_size *2)/2 - (hex_size*50);
    sf::View view(sf::FloatRect((hex_size *arr_size * 2)/2 - tomid_offset, (hex_size *arr_size *2)/2 - tomid_offset, hex_size *arr_size *8, hex_size *arr_size *8));

    // activate it
    //window.setView(view);


    int drawn = 0;

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
            
            if(event.type == sf::Event::KeyPressed) {
                if(event.key.code == sf::Keyboard::L) {
                    window.clear({60, 60, 60, 255});
                    for(int i = 1; i < arr_size-1; i++){
                        // printf("i: %d\n", i);
                        for(int j = 1; j < arr_size-1; j++){
                            if(hex_grid[i][j].s >= 1){
                                set_shape_hexcell(&shape, hex_size, i, j);
                                window.draw(shape);
                            }
                        }
                    }
                }
            }
        }
        window.display();
    }
}

void evenq_offset_to_pixel(double *x, double *y, int size, int i, int j){
    // *x = (size) * ((double)3/2) * j;
    // *y = (size) * sqrt(3) * ((double)i - 0.5 * (j % 2));

    *x = (double)size * sqrt(3) * ((double)j - 0.5 * (i % 2));
    *y = (double)size * ((double)3/2) * i;

    return;
}
