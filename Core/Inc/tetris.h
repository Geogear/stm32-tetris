#ifndef TETRIS_H
#define TETRIS_H

#include "stdint.h"
#define SCALE 4
#define SCREEN_X 64
#define SCREEN_Y 128
#define TETRIS_ROW (SCREEN_Y / SCALE) // 32
#define TETRIS_COLUMN (SCREEN_X / SCALE) // 16

// We assume 0,0 coord to be in tpo left of the screen.

union shape_map{
	uint16_t val;
	struct{
		uint8_t row1 : 4;
		uint8_t row2 : 4;
		uint8_t row3 : 4;
		uint8_t row4 : 4;
	};
};

enum SHAPE{
	SQUARE = 0,
	ROD = 1,
	Z_BLOCK = 2,
	L_BLOCK = 3,
	T_BLOCK = 4,
	SHAPE_COUNT = 5
};

enum SM_MOVE{
	LEFT = 0,
	RIGHT = 1,
	DOWN = 2,
	ROTATE = 3,
	NO_INPUT = 4
};

enum GAME_STATE{
	START = 0,
	RUNNING = 1,
	LOST = 2
};

// Display coords are 128x64, tetris coords are 64x128 so we're just switching the
// x and y to use the screen vertically.
// Sets the coord as, pixel exists
//void tetris_set(uint8_t y, uint8_t x);
// Returns 1 if pixel exists, 0 if not, need to be multiplied with the color value.
//uint8_t tetris_get(uint8_t y, uint8_t x);
//void tetris_clear(void);

void sm_set_shape(union shape_map* sm, enum SHAPE shape, uint8_t take_mirror);
void sm_set(union shape_map* sm, uint8_t y, uint8_t x);
uint8_t sm_get(union shape_map* sm, uint8_t y, uint8_t x);
void sm_rotate(union shape_map* sm, uint8_t count);

void put_shape_on_map(union shape_map* sm);
void display_map(void);
void sm_set_coord(uint8_t y, uint8_t x);
void rng_seed(uint32_t seed);
enum GAME_STATE game_iteration(enum SM_MOVE move);
uint32_t get_score(void);
void init_game(void);

#endif
