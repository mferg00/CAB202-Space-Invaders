// sprite index in array
#define STARFIGHTER 0
#define ASTEROID 1
#define BOULDER 2
#define FRAGMENT 3
#define HEART 4
#define BEAM 5

//object dimensions
#define SF_WIDTH 9
#define SF_HEIGHT 4
#define AST_SIZE 7
#define BOL_SIZE 5
#define FRAG_SIZE 3

// modified from the draw_char function in graphics.c to suit the SPRITE array
void draw_sprite(int spritenum, int top_left_x, int top_left_y, colour_t colour);

// draw barrier every 2nd pixel at y 39
void draw_barrier();
