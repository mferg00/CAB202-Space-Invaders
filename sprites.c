#include <stdio.h>
#include <avr/pgmspace.h>

#include "graphics.h"
#include "sprites.h"

//8bit sprite values to be stored in program memory to save ram
//8bit so that pgm_read_byte() can be used. pgm_read_word() could be used for 16 bit
static const uint8_t SPRITES[][9] PROGMEM = {
  {0b1000, 0b1100, 0b1100, 0b1111, 0b1111, 0b1111, 0b1100, 0b1100, 0b1000}, //0: starfighter
  {0b0001000, 0b0011100, 0b0111110, 0b1111111, 0b0111110, 0b0011100, 0b0001000}, //1: asteroid
  {0b00100, 0b01110, 0b11111, 0b01110, 0b00100}, //2: boulder
  {0b010, 0b111, 0b010}, //3: fragment
  {0b000110, 0b001111, 0b011111, 0b110110, 0b011011, 0b001111, 0b000110}, //4: heart
  {0b11, 0b11} //5: beam
};

// modified from the draw_char function in graphics.c to suit the SPRITE array
void draw_sprite(int spritenum, int top_left_x, int top_left_y, colour_t colour){

  int width = 0, height = 0;

  // get the width and height for each sprite
  switch(spritenum){
    case STARFIGHTER: width = SF_WIDTH; height = SF_HEIGHT; break;
    case ASTEROID: width = AST_SIZE; height = AST_SIZE; break;
    case BOULDER: width = BOL_SIZE; height = BOL_SIZE; break;
    case FRAGMENT: width = FRAG_SIZE; height = FRAG_SIZE; break;
    case HEART: width = 7; height = 6; break;
    case BEAM: width = 2; height = 2; break;
  }

  // for each row of the sprite from 0 to width, read its value
	for ( uint8_t i = 0; i < width; i++ ) {
		uint8_t pixel_data = pgm_read_byte(&(SPRITES[spritenum][i]));

    // for each column of the sprite from 0 to height, place a pixel at j in pixel_data
		for ( uint8_t j = 0; j < height; j++ ) {
			draw_pixel(top_left_x + i, top_left_y + j, (pixel_data & (1 << j)) >> j);
		}
	}
}

// draw barrier every 2nd pixel at y 39
void draw_barrier(){
  for(int i = 0; i < LCD_X; ++i){
    if(i % 2 == 0){
      draw_pixel(i, 39, FG_COLOUR);
    }
  }
}
