//#ifndef DRAW_HEX_GRID_OFFSET_ARRAY_GUARD
//#define DRAW_HEX_GRID_OFFSET_ARRAY_GUARD

#pragma once

#include <stdlib.h>
#include <math.h>
#include "hex.h"

#include <SFML/Graphics.hpp>

struct point{
    double x;
    double y;
};


void write_to_diskimg(int hex_size, int arr_size, cell **hex_grid);
void show_on_screen(const int hex_size, const int arr_size, cell **hex_grid, const int frozen_cells_cnt);

int shape_hexgrid(sf::CircleShape *shape_arr, const double hex_size, const int arr_size, cell **hex_grid);
void set_shape_hexcell(sf::CircleShape *shape, const double size, int i, int j);
void draw_on_screen(sf::CircleShape *arr, const int len, int window_size);
void calc_draw(cell **hex_grid, int arr_size, double hex_size);
void evenq_offset_to_pixel(double *x, double *y, int size, int i, int j);

//#endif DRAW_HEX_GRID_OFFSET_ARRAY_GUARD