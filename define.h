//apparently stdbool.h isnt allowed so just do this because this is what bool is but just less intuitive
#define true 1
#define false 0
#define bool int

// helper macros
#define rand_range(min, max) ((rand() % (max - min + 1)) + min)  //get a random value inbetween x and y (inclusive)
#define send_str_P(x) (send_str_progmem(PSTR(x)))   //shortcut to store a string in PROGMEM (to save precious RAM) and send it to usb serial out. send_str_progmem() is defined in serial_io.c
#define BIT(x) (1 << (x))  //set bit shortcut

//clock speed values
#define PRESCALE 1024.0
#define FREQ 8000000.0

//cannon dimensions
#define CANNON_WIDTH 5
#define CANNON_HEIGHT 3

//max amount of beams allowed (errors occur above 50, so go 30 for safety)
#define MAX_BEAMS 30

//max amount of invaders (if you change this things will break)
#define MAX_ASTEROIDS 3
#define MAX_BOULDERS 6
#define MAX_FRAGMENTS 12

//game lives to start with
#define LIVES 2
