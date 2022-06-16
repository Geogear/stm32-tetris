#include "tetris.h"
#include "ssd1306.h"

uint16_t map[TETRIS_ROW] = {};
static union shape_map sm_cur = {};
// Map current coords of the, top left coord of cur_sm.
static uint8_t sm_cur_y = 0;
static uint8_t sm_cur_x = 0;
static uint8_t lock = 0;
static uint32_t rng_state = 0x31;
static uint32_t last_time = 0;
static uint32_t interval = 500;
static uint32_t score = 0;

static inline void tetris_set(uint8_t y, uint8_t x){
	map[y] = map[y] ^ (0x1 << x);
}

static inline uint8_t tetris_get(uint8_t y, uint8_t x){
	return (map[y] >> x) & 0x1;
}

static inline void tetris_clear(void){
	for(uint8_t i = 0; i < TETRIS_ROW; ++i)
		map[i] = 0;
}

static void sm_get_borders(uint8_t* y, uint8_t* x_left, uint8_t* x_right){
	//TODO
}

static uint8_t sm_bottom_hits(void){
	/*uint8_t bottom_y = 100;
	uint8_t bottom_row = 0;
	uint8_t hits = 0;

	if(sm_cur.row4 != 0){
		bottom_y = 3; bottom_row = sm_cur.row4;
	}else if(sm_cur.row3 != 0){
		bottom_y = 2; bottom_row = sm_cur.row3;
	}else if(sm_cur.row2 != 0){
		bottom_y = 1; bottom_row = sm_cur.row2;
	}else{
		bottom_y = 0; bottom_row = sm_cur.row1;
	}

	// Convert to map coord
	bottom_y += sm_cur_y;
	if(bottom_y == TETRIS_ROW-1){
		hits = 1;
	}else{
		for(uint8_t i = 0; i < 4; ++i){
			// If at this row and column, the sm has a pixel
			// and at one below there is a pixel, it hits.
			uint8_t pixel = (bottom_row >> (3-i)) & 0x1;
			if(pixel && tetris_get(bottom_y+1, i + sm_cur_x)){
				hits = 1;
				break;
			}
		}
	}

	return hits;*/

	uint8_t bottom_y = 100;
	if(sm_cur.row4 != 0){
		bottom_y = 3;
	}else if(sm_cur.row3 != 0){
		bottom_y = 2;
	}else if(sm_cur.row2 != 0){
		bottom_y = 1;
	}else{
		bottom_y = 0;
	}

	if(sm_cur_y + bottom_y == TETRIS_ROW-1)
		return 1;

	uint8_t blocked = 0, pixel_below = 0;
	for(uint8_t i = 0; i < 4; ++i){
		for(uint8_t j = 0; j < 4; ++j){
			uint8_t bit = sm_get(&sm_cur, i ,j);
			if(bit && i != 3){
				pixel_below = sm_get(&sm_cur, i+1 ,j);
			}
			if(bit && (i == 3 || !pixel_below)){
				uint8_t pos_x = sm_cur_x + j;
				if(tetris_get(sm_cur_y + i + 1, pos_x)){
					blocked = 1;
					break;
				}
			}
		}
		if(blocked)
			break;
	}
	return blocked;
}

void sm_set_shape(union shape_map* sm, enum SHAPE shape, uint8_t take_mirror){
	sm->val = 0;
	switch(shape){
		case SQUARE:
			sm_set(sm, 0, 0); sm_set(sm, 0, 1); // first row
			sm_set(sm, 1, 0); sm_set(sm, 1, 1); // second row
			break;
		case ROD:
			for(uint8_t i = 0; i < 4; ++i)
				sm_set(sm, i, 0); // always at first column and go down the rows
			break;
		case Z_BLOCK:
				sm_set(sm, 0, 0); sm_set(sm, 0, 1);
				sm_set(sm, 1, 1); sm_set(sm, 1, 2);
			break;
		case L_BLOCK:
			for(uint8_t i = 0; i < 4; ++i)
				sm_set(sm, i, 0);
			sm_set(sm, 3, 1);
			break;
		case T_BLOCK:
			for(uint8_t i = 0; i < 3; ++i)
				sm_set(sm, 0, i); // fill the first 3 colum on the first row
			sm_set(sm, 1, 1);
			break;
	}

	if(take_mirror){
		union shape_map take_inverse;
		take_inverse.val = sm->val;
		sm->val = 0;
		for(uint8_t i = 0; i < 4; ++i){
			for(uint8_t j = 0; j < 4; ++j){
				// If there is a set pixel value on the coord.
				// Set y mirrored of that coord.
				if(sm_get(&take_inverse, i, j)){
					sm_set(sm, i, 3-j);
				}
			}
		}
	}
}

// 4x4 sm.
void sm_set(union shape_map* sm, uint8_t y, uint8_t x){
	switch(y){
		case 0:
			sm->row1 = sm->row1 ^ (0x1 << (3-x));
			break;
		case 1:
			sm->row2 = sm->row2 ^ (0x1 << (3-x));
			break;
		case 2:
			sm->row3 = sm->row3 ^ (0x1 << (3-x));
			break;
		case 3:
			sm->row4 = sm->row4 ^ (0x1 << (3-x));
			break;
	}
}

uint8_t sm_get(union shape_map* sm, uint8_t y, uint8_t x){
	uint8_t pixel = 0;
	switch(y){
		case 0:
			pixel = (sm->row1 >> (3-x)) & 0x1;
			break;
		case 1:
			pixel = (sm->row2 >> (3-x)) & 0x1;
			break;
		case 2:
			pixel = (sm->row3 >> (3-x)) & 0x1;
			break;
		case 3:
			pixel = (sm->row4 >> (3-x)) & 0x1;
			break;
	}
	return pixel;
}

static uint8_t sm_get_reverse_column(union shape_map* sm, uint8_t column_index){
	uint8_t column = 0;
	column_index = 3 - column_index;

	column = column ^ (((sm->row1 >> column_index) & 0x1) << 0);
	column = column ^ (((sm->row2 >> column_index) & 0x1) << 1);
	column = column ^ (((sm->row3 >> column_index) & 0x1) << 2);
	column = column ^ (((sm->row4 >> column_index) & 0x1) << 3);

	return column;
}

// Rotates the sm in the clock wise direction with 90 degrees count%4 times.
void sm_rotate(union shape_map* sm, uint8_t count){
	uint8_t actual_count = count % 4;
	union shape_map to_rotate;
	for(uint8_t i = 0; i < actual_count; ++i){
		// 90degrees clock wise rotation.
		to_rotate.val = sm->val;
		sm->row1 = sm_get_reverse_column(&to_rotate, 0);
		sm->row2 = sm_get_reverse_column(&to_rotate, 1);
		sm->row3 = sm_get_reverse_column(&to_rotate, 2);
		sm->row4 = sm_get_reverse_column(&to_rotate, 3);
	}
}

void put_shape_on_map(union shape_map* sm){
	uint8_t map_x = 0, map_y = 0;
	for(uint8_t i = 0; i < 4; ++i){
		map_y = sm_cur_y + i;
		for(uint8_t j = 0; j < 4; ++j){
			uint8_t bit = sm_get(sm, i, j);
			if(bit){
				map_x = sm_cur_x + j;
				tetris_set(map_y, map_x);
			}
		}
	}
}

// Displays a live shape without putting it in the map
static void display_shape(void){
	uint16_t scaled_y = 0, scaled_x = 0;
	uint8_t map_y = 0, map_x = 0;
	for(uint8_t i = 0; i < 4; ++i){
		map_y = sm_cur_y + i;
		for(uint8_t j = 0; j < 4; ++j){
			uint8_t pixel = sm_get(&sm_cur, i, j);
			if(pixel){
				map_x = sm_cur_x + j;
				for(uint8_t si = 0; si < 4; ++si){
					scaled_y = map_y*SCALE + si;
					for(uint8_t sj = 0; sj < 4; ++sj){
						scaled_x = map_x*SCALE  + sj;
						SSD1306_DrawPixel(scaled_y, scaled_x, 1);
					}
				}
			}
		}
	}
}

// At the end when putting the actual pixels, shuffle x and y (is that enough?)
// decide the piece color (just white for now) and scale up.
void display_map(void){
	uint16_t scaled_y = 0, scaled_x = 0;
	for(uint8_t i = 0; i < TETRIS_ROW; ++i){
		for(uint8_t j = 0; j < TETRIS_COLUMN; ++j){
			uint8_t bit = tetris_get(i, j);
			if (bit){
				// Coord conversion according to the scale
				for(uint8_t s_i = 0; s_i < SCALE; ++s_i){
					scaled_y = i*SCALE + s_i;
					for(uint8_t s_j = 0; s_j < SCALE; ++s_j){
						scaled_x = j*SCALE + s_j;
						// X and Y shifted
						SSD1306_DrawPixel(scaled_y, scaled_x, 1);
					}
				}
			}
		}
	}
}

void sm_set_coord(uint8_t y, uint8_t x){
	sm_cur_y = y;
	sm_cur_x = x;
}

static uint8_t right_blocked(void){
	uint8_t blocked = 0;
	for(uint8_t i = 0; i < 4; ++i){
		for(uint8_t j = 4; j > 0; --j){
			uint8_t bit = sm_get(&sm_cur, i ,j-1);
			if(bit){
				uint8_t pos_x = sm_cur_x + j-1;
				if(pos_x == TETRIS_COLUMN-1 || tetris_get(sm_cur_y + i,pos_x+1))
					blocked = 1;
				break;
			}
		}
		if(blocked)
			break;
	}
	return blocked;
}

static uint8_t left_blocked(void){
	uint8_t blocked = 0;
	for(uint8_t i = 0; i < 4; ++i){
		for(uint8_t j = 0; j < 4; ++j){
			uint8_t bit = sm_get(&sm_cur, i ,j);
			if(bit){
				uint8_t pos_x = sm_cur_x + j;
				if(pos_x == 0 || tetris_get(sm_cur_y + i,pos_x-1))
					blocked = 1;
				break;
			}
		}
		if(blocked)
			break;
	}
	return blocked;
}

void sm_update(enum SM_MOVE move){
	//TODO add screen borders
	// TODO add borders for other existing pixels on the right or the left
	switch(move){
		case LEFT:
			if(!left_blocked())
				sm_cur_x -= 1;
			break;
		case RIGHT:
			if(!right_blocked())
				sm_cur_x += 1;
			break;
		case DOWN:
				if (lock)
					return;
				if(!sm_bottom_hits())
					++sm_cur_y;
				else
					lock = 1;
			break;
		case ROTATE:
			sm_rotate(&sm_cur, 1);
			break;
	}
}

void rng_seed(uint32_t seed){
	rng_state = seed;
}

static uint32_t rng(void){
	rng_state = rng_state * 1664525 + 1620847577;
	return rng_state;
}

static void check_elimination(void){
	uint8_t rows_to_eliminate = 0;
	uint8_t start_row = 200;
	for(uint8_t i = TETRIS_ROW; i > 0; --i){
		if(map[i-1] == 0xffff){
			++rows_to_eliminate;
			if(start_row == 200)
				start_row = i-1;
		}
	}
	if(rows_to_eliminate){
		score += rows_to_eliminate * TETRIS_COLUMN;
		for(uint8_t i = start_row+1; i > rows_to_eliminate; --i){
			map[i-1] = map[i-1 - rows_to_eliminate];
		}
		for(uint8_t i = 0; i < rows_to_eliminate; ++i)
			map[i] = 0;
	}
}

// TODO add color input,
enum GAME_STATE game_iteration(enum SM_MOVE move){
	uint8_t go_down = 0;

	if(last_time + interval < HAL_GetTick()){
		last_time = HAL_GetTick();
		go_down = 1;
	}

	if(go_down || move != NO_INPUT)
		SSD1306_Clear();

	if(tetris_get(0, 8))
		return LOST;

	// New shape, init it.
	if(sm_cur_x == 0 && sm_cur_y == 0){
		sm_cur_x = 8;
		uint32_t shape = rng() % SHAPE_COUNT;
		sm_set_shape(&sm_cur, shape, 0);
	}else{
		sm_update(move);
	}

	//go down one.
	if(go_down)
		sm_update(DOWN);
	// We've hit the bottom.
	if(lock){
		// Put shape data on the map.
		put_shape_on_map(&sm_cur);
		// Reset the vals.
		sm_cur_x = sm_cur_y = lock = 0;
		// Check elimination
		check_elimination();
	}else{
		// Shape is not locked, display it separately
		display_shape();
	}
	// Display the map
	if(go_down || move != NO_INPUT){
		display_map();
		SSD1306_UpdateScreen();
	}
	return RUNNING;
}

uint32_t get_score(void){
	return score;
}

void init_game(void){
	score = 0;
	sm_cur_y = sm_cur_x = 0;
	for(uint32_t i = 0; i < TETRIS_ROW; ++i)
		map[i] = 0;
}
