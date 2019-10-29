#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <cpu_speed.h>
#include <graphics.h>
#include <lcd.h>
#include <macros.h>
#include "lcd_model.h"
#include "usb_serial.h"

#include "define.h"
#include "structs.h"
#include "hardware.h"
#include "serial_io.h"
#include "sprites.h"

//cannon x and y values (at end of cannon, start of cannon snaps to starfighter)
int cx, cy;

starfighter_data starfighter;

beam_data beam[MAX_BEAMS];

invader_data asteroid[3]; invader_data boulder[6]; invader_data fragment[12];

status_data status;

input_data input;

void send_status(void);

//
//  BUTTON/SWITCH INPUT and DEBOUNCING
//

// decide the state of the pin (on/off) based on the pin history
// note that any input with a lock_[input] variable will only work for clicking, but not holding. e.g. holding down joystick r will send 1000...., not 1111....
// this is to prevent flickering between states (paused/status screen), or difficulty in making the starfighter stationary
void switchstates(uint8_t mask) {

  if(input_check(0, mask)){
    input.joystick_l = 1;
  }

  else{
    input.lock_joystick_l = false;
    input.joystick_l = 0;
  }

  if(input_check(1, mask)){
    input.joystick_r = 1;
  }

  else{
    input.lock_joystick_r = false;
    input.joystick_r = 0;
  }

  if(input_check(2, mask)){
    input.joystick_u = 1;
  }

  else{
    input.joystick_u = 0;
  }

  if(input_check(3, mask)){
    input.joystick_d = 1;
  }

  else{
    input.lock_joystick_d = false;
    input.joystick_d = 0;
  }

  if(input_check(4, mask)){
    input.joystick_c = 1;
  }

  else{
    input.lock_joystick_c = false;
    input.joystick_c = 0;
  }

  if(input_check(5, mask)){
    input.sw1 = 1;
  }

  else{
    input.sw1 = 0;
  }

  if(input_check(6, mask)){
    input.sw2 = 1;
  }

  else{
    input.sw2 = 0;
  }

}

// grab state of the pins every time timer0 overflows
ISR(TIMER0_OVF_vect) {

  uint8_t mask = 0b00011111;
  input_history(mask);
  switchstates(mask);

}

//
//  CLEAR ITEM FUNCTIONS
//

void clear_asteroid(int i){

  asteroid[i].active = false;
  asteroid[i].x = -100;
  asteroid[i].y = -100;
  asteroid[i].dy = 0;
  asteroid[i].dx = 0;
  --status.asteroids;
}

void clear_beam(int i){

  beam[i].active = false;
  beam[i].dir = 0;
  beam[i].x = 0;
  beam[i].y = 0;
  beam[i].dx = 0;
  beam[i].dy = 0;
    --status.beams;
}

void clear_beams() {

  for (int i = 0; i < MAX_BEAMS; ++i){
    clear_beam(i);
  }

  status.latest_beam = 0;
  beam[0].fired_at = status.time;
  status.beams = 0;

}

void clear_boulder(int i){

  boulder[i].active = false;
  boulder[i].dir = 0;
  boulder[i].dx = 0;
  boulder[i].dy = 0;
  boulder[i].x = 0;
  boulder[i].y = 0;
    --status.boulders;

}

void clear_boulders(){

  for(int i = 0; i < 6; ++i){
    clear_boulder(i);
  }

  status.boulders = 0;
}

void clear_fragment(int i){

  fragment[i].active = false;
  fragment[i].dir = 0;
  fragment[i].dx = 0;
  fragment[i].dy = 0;
  fragment[i].x = 0;
  fragment[i].y = 0;
    --status.fragments;

}

void clear_fragments(){

  for(int i = 0; i < 12; ++i){
    clear_fragment(i);
  }

  status.fragments = 0;
}

//
//  SETUP ITEM FUNCTIONS
//

//find the index of an inactive asteroid
int find_inactive_asteroid(){
  //find an inactive boulder
  for(int i = 0; i < MAX_ASTEROIDS; ++i){
    if(!asteroid[i].active){
      return i;
    }
  }
  return 0;
}

int find_inactive_boulder(){
  //find an inactive boulder
  for(int i = 0; i < MAX_BOULDERS; ++i){
    if(!boulder[i].active){
      return i;
    }
  }
  return 0;
}

int find_inactive_fragment(){
  //find an inactive boulder
  for(int i = 0; i < MAX_FRAGMENTS; ++i){
    if(!fragment[i].active){
      return i;
    }
  }
  return 0;
}

int find_inactive_beam(){
  //find an inactive boulder
  for(int i = 0; i < MAX_BEAMS; ++i){
    if(!beam[i].active){
      return i;
    }
  }
  return 0;
}

// sets a beam[i] up with the velocity based on the turret aim
void setup_beam(int i){

  beam[i].dir = status.aim;
  beam[i].x = cx;
  beam[i].y = cy - 2;
  beam[i].dx = 0.2 * sin(beam[i].dir * M_PI/180);
  beam[i].dy = 0.2 * cos(beam[i].dir * M_PI/180);
  beam[i].active = true;
  beam[i].fired_at = status.time;

  status.latest_beam = i;
  ++status.beams;
}

// finds a beam[i] that isn't active for it's values to be replaced and become active again
void replace_beam(){
  setup_beam(find_inactive_beam());
}

// sets up a pair of boulders with velocities from -60 to 60
void setup_boulderpair(int ast_id){

  int i = find_inactive_boulder();

  boulder[i].dir = rand_range(0, 30);
  boulder[i].dx = sin(boulder[i].dir * M_PI/180);
  boulder[i].dy = cos(boulder[i].dir * M_PI/180);
  boulder[i].x = asteroid[ast_id].x + 5;
  boulder[i].y = asteroid[ast_id].y + 4;
  boulder[i].active = true;
  ++status.boulders;

  i = find_inactive_boulder();

  boulder[i].dir = rand_range(-30, 0);
  boulder[i].dx = sin(boulder[i].dir * M_PI/180);
  boulder[i].dy = cos(boulder[i].dir * M_PI/180);
  boulder[i].x = asteroid[ast_id].x;
  boulder[i].y = asteroid[ast_id].y + 4;
  boulder[i].active = true;
  ++status.boulders;

}

// sets up a pair of fragments with velocities from -60 to 60 relative to the velocity of the boulder
void setup_fragmentpair(int bol_id){

  int i = find_inactive_fragment();

  fragment[i].dir = boulder[bol_id].dir + rand_range(0, 30);
  fragment[i].dx = sin(fragment[i].dir * M_PI/180);
  fragment[i].dy = cos(fragment[i].dir * M_PI/180);
  fragment[i].x = boulder[bol_id].x;
  fragment[i].y = boulder[bol_id].y + 3;
  fragment[i].active = true;
  ++status.fragments;

  i = find_inactive_fragment();

  fragment[i].dir = boulder[bol_id].dir - rand_range(0, 30);
  fragment[i].dx = sin(fragment[i].dir * M_PI/180);
  fragment[i].dy = cos(fragment[i].dir * M_PI/180);
  fragment[i].x = boulder[bol_id].x + 3;
  fragment[i].y = boulder[bol_id].y + 3;
  fragment[i].active = true;
  ++status.fragments;

}

//
//  INITIAL SETUP
//

// enable manual mode for potentiometers
void setup_potentiometer(){
  input.pot0_enabled = true;
  input.pot1_enabled = true;
}

// setup an asteroid at a random x value just above the screen
void setup_asteroid(int i, int rand_xpos){
  asteroid[i].x = rand_xpos;
  asteroid[i].y = -AST_SIZE;
  asteroid[i].active = true;
}

// check to see if the random x value collides with a previous asteroid
bool check_asteroid(int i, int rand_xpos){

  if(i == 1 && rand_xpos > asteroid[i-1].x - AST_SIZE && rand_xpos < asteroid[i-1].x + AST_SIZE){
    return false;
  }

  else if((rand_xpos > asteroid[i-1].x - AST_SIZE && rand_xpos < asteroid[i-1].x + AST_SIZE) ||
            (rand_xpos > asteroid[i-2].x - AST_SIZE && rand_xpos < asteroid[i-2].x + AST_SIZE)) {
    return false;
  }

  return true;
}

// get a random x value and check if it doesnt collide with an asteroid. set it up if it doesn't or try again if it does
void setup_asteroids(){

  LEDBOTH_OFF;

  //rand x pos within screen bounds
  int rand_xpos = 1 + rand() % (LCD_X - 1 - AST_SIZE);

  //setup asteroid 0 beacause it cant collide with anything
  setup_asteroid(0, rand_xpos);

  for(int i = 1; i < MAX_ASTEROIDS; ){
    rand_xpos = 1 + rand() % (LCD_X - 1 - AST_SIZE);
    if(check_asteroid(i, rand_xpos)){
      setup_asteroid(i, rand_xpos);
      i++;
    }
    else if(!check_asteroid(i, rand_xpos)){
      i--;
    }
  }

  status.asteroids = MAX_ASTEROIDS;

}

// setup starfighter in center bottom screen with a random velocity left or right
void setup_starfighter(){
  starfighter.stationary = false;
  starfighter.left = false;
  starfighter.right = false;
  starfighter.x = LCD_X / 2 - 4;
  starfighter.y = LCD_Y - 4;

  switch(rand() % 2){
    case 1: starfighter.left = true; break;
    case 0: starfighter.right = true; break;
  }

}

// setup status values
void setup_status(){
  status.time = 0;
  status.start_time = status.time;
  status.clock[0] = 0; status.clock[1] = 0; status.clock[2] = 0;
  status.first_resume = true;
  status.beams = 0;
  status.asteroids = 0;
  status.boulders = 0;
  status.fragments = 0;
  status.score = 0;
  status.shield = 5;
  status.paused = true;
  status.show = false;
}

// setup hardware that only requires to be setup once at the initial boot up of the teensy
void setup_hardware(){

  setup_pot();
  setup_led();
  setup_timer0();
  setup_pwm();
  sei(); //enable interrupts

  draw_string(10, 10, "Connect USB...", FG_COLOUR);
  show_screen();

  while ( !usb_configured() ) {
    // Block until USB is ready.
  }
  clear_screen();
}

// screen that shows lives remaining every time the game is reset
void new_game(){

  clear_screen();
  LCD_CMD(lcd_set_display_mode, lcd_display_inverse);

  draw_sprite(HEART, (LCD_X/2) - 23, LCD_Y / 2, 1);
  char buffer[6]; snprintf(buffer, sizeof(buffer), "x  %d", status.lives);
  draw_string((LCD_X/2) - 3, LCD_Y/2, buffer, 1);

  show_screen();
  _delay_ms(1000);
  clear_screen();

  LCD_CMD(lcd_set_display_mode, lcd_display_normal);

}

// values to change and functions to run every time the game restarts
void setup() {

  setup_potentiometer();
  setup_status();
  clear_beams();
  clear_boulders();
  clear_fragments();
  setup_starfighter();
  setup_asteroids();
  new_game();

}


// quit screen if 'q' or sw2 is pressed
void quit(){

  clear_screen();
  LCD_CMD(lcd_set_display_mode, lcd_display_inverse);
  draw_string(5,5, "n10226052", 1);
  show_screen();
  exit(0);
}


// game over screen when lives == 0
void game_over(){

  clear_screen();

  // fade backlight down
  for(int i = 0; i < 1024; ++i){
    set_backlight(i);
    _delay_ms(2);
  }

  LED1_ON; LED0_ON;
  draw_string(15, 5, "game over", FG_COLOUR);
  show_screen();
  _delay_ms(2000);

  LEDBOTH_OFF;

  //fade backlight up
  for(int i = 1023; i >= 0; --i){
    set_backlight(i);
    _delay_ms(2);
  }

  clear_screen();
  draw_string(15, 5, "game over", FG_COLOUR);
  draw_string(15, 20, "restart?", 1);
  draw_string(15, 35, "quit?", 1);
  show_screen();

  //wait for input
  while(1){

    int16_t char_code = usb_serial_getchar();
    char ch = char_code;
    if(input.sw1 == 1 || ch == 'r'){
      status.lives = LIVES;
      setup();
      break;
    }
    else if(input.sw2 == 1 || ch == 'q'){
      quit();
      break;
    }
  }

}

// startup screen to be run only once when the teensy is powered on
void startup_screen(){

  int led_val = 0;
  int led_nextval = 1023;
  int line_end = 5;
  int rand_seed = 0;

  while(1){

    for(int i = 0; i < 6; ++i){
      clear_screen();
      set_backlight(led_val); //will flicker between on and off
      led_val ^= led_nextval; //swap values (led_val = 0 <-> 1023)
      _delay_ms(500);
      line_end += (LCD_X - 10) / 6;
      draw_string(5, 35, "loading...", 1);
      draw_line(5, LCD_Y - 5, line_end, LCD_Y - 5, 1);
      draw_string(5, 5, "n10226052", 1);
      draw_string(5, 15, "space invaders", 1);
      show_screen();
    }
    break;
  }

  set_backlight(0);

  while(1){
    //get the rand seed based on timer0 (while button isnt pressed)
    rand_seed += TCNT0;
    int16_t char_code = usb_serial_getchar();
    char ch = char_code;
    if(input.sw1 == 1 || ch == 'r') {
      srand(rand_seed);
      setup();
      break;
    }

    clear_screen();
    draw_line(5, LCD_Y - 5, line_end, LCD_Y - 5, 1);
    draw_string(5, 35, "press to play", 1);
    draw_string(5, 5, "n10226052", 1);
    draw_string(5, 15, "space invaders", 1);
    show_screen();
  }

}

// setup to be run once, when the teensy is powered on
void init_setup(){

  set_clock_speed(CPU_8MHz);
  lcd_init(LCD_DEFAULT_CONTRAST);
  lcd_clear();
  usb_init();
  setup_hardware();
  status.lives = LIVES;
  startup_screen();
  setup();

}

//
//  COLLISION DETECTION
//

// checks if an asteroid has collided with the barrier (pixel level)
void barrier_asteroid_collision(invader_data *a, int ast_id){

  int x = round(a->x) + 4;
  int y = round(a->y) + AST_SIZE;

  /*   checks for this collision
      invader:    --,_,--
      barrier:    --   --
  */
  if(x % 2 == 0){
    if(y == 40){
      clear_asteroid(ast_id);
      --status.shield;
    }
  }
  /*   checks for this collision
      invader:    --,_,--
      barrier: --   --   --
  */
  else if((x+1) % 2 == 0){
    if(y == 39){
      clear_asteroid(ast_id);
      --status.shield;
    }
  }
}

// checks all asteroids
void barrier_asteroid_collisions(){
  for(int i = 0; i < MAX_ASTEROIDS; ++i){
    if(asteroid[i].active){
      barrier_asteroid_collision(&asteroid[i], i);
    }
  }
}

void barrier_boulder_collision(invader_data *a, int bol_id){

  int x = round(a->x) + 3;
  int y = round(a->y) + BOL_SIZE;

  if(x % 2 == 0){
    if(y == 40){
      clear_boulder(bol_id);
      --status.shield;
    }
  }

  else if((x+1) % 2 == 0){
    if(y == 39){
      clear_boulder(bol_id);
      --status.shield;
    }
  }
}

void barrier_boulder_collisions(){
  for(int i = 0; i < MAX_BOULDERS; ++i){
    if(boulder[i].active){
      barrier_boulder_collision(&boulder[i], i);
    }
  }
}

void barrier_fragment_collision(invader_data *a, int frag_id){

  int x = round(a->x) + 1;
  int y = round(a->y) + FRAG_SIZE;

  if(x % 2 == 0){
    if(y == 40){
      clear_fragment(frag_id);
      --status.shield;
    }
  }

  else if((x+1) % 2 == 0){
    if(y == 39){
      clear_fragment(frag_id);
      --status.shield;
    }
  }
}

void barrier_fragment_collisions(){
  for(int i = 0; i < MAX_FRAGMENTS; ++i){
    if(fragment[i].active){
      barrier_fragment_collision(&fragment[i], i);
    }
  }
}

// check to see if the beam collides with the asteroid
// ALL beam/invader collision detections are pixel level, as they check with a diamond shaped bounding box instead of square
// ill try to explain how this works in the function below (this explanation applies to the other beam/invader collisions)
void beam_asteroid_collision(beam_data *b, invader_data *a, int ast_id, int beam_id){

  // the coordinate of a pixel is (+0.5, +0.5) inwards of the top left corner of the pixel, see diagram below for a pathetic explanation
  /*                    PIXEL:
        top left corner -> +-+
                           +`+ -> ` = center of pixel, and where coordinates are
  */
  // so to make a bounding box around the center of the beam, we need to move the coordinates +0.5 inwards (x and y)
  /*    BEAM:
        +-+-+
        +`o-+  -> o = center of beam, ` = beam coordinates
        +-+-+

  */

  //this also applies to the asteroid (and all other invaders). Because the invaders are all odd-number sized, their center will not be in the corners of four pixels (like the beam), but just in the center of one pixel
  // so the center of an asteroid will be +3 inwards (x and y)

  //the formula below checks if the center of the beam is less than or equal to 3.5 (3+0.5) pixels away from the center of the asteroid (and therefore collided)
  // because the values are rounded, it becomes a diamond shaped collision boudnary. If they weren't rounded it would be circle based collisiond detection.
  if((abs((a->x+3)-(round(b->x)+0.5)) + abs((round(a->y)+3)-(round(b->y)+0.5))) <= 3.5){
    ++status.score;
    clear_beam(beam_id);
    setup_boulderpair(ast_id);
    clear_asteroid(ast_id);
  }

}

//checks collision of each beam against each asteroid
void beam_asteroid_collisions(int beam_id){
  for(int ast_id = 0; ast_id < MAX_ASTEROIDS; ast_id++){
    if(asteroid[ast_id].active){
      beam_asteroid_collision(&beam[beam_id], &asteroid[ast_id], ast_id, beam_id);
    }
  }
}


void beam_boulder_collision(beam_data *b, invader_data *a, int bo_id, int beam_id){

  if((abs(round(a->x+2)-(round(b->x)+0.5))+ abs((round(a->y)+2)-(round(b->y)+0.5))) <= 2.5){
    status.score += 2;
    clear_beam(beam_id);
    setup_fragmentpair(bo_id);
    clear_boulder(bo_id);
  }

}

void beam_boulder_collisions(int beam_id){
  for(int bo_id = 0; bo_id < MAX_BOULDERS; bo_id++){
    if(boulder[bo_id].active){
      beam_boulder_collision(&beam[beam_id], &boulder[bo_id], bo_id, beam_id);
    }
  }
}


void beam_fragment_collision(beam_data *b, invader_data *a, int frag_id, int beam_id){

  if((abs((round(a->x)+1)-(round(b->x)+0.5))+ abs((round(a->y)+1)-(round(b->y)+0.5))) <= 1.5){
    status.score += 4;
    clear_beam(beam_id);
    clear_fragment(frag_id);
  }

}

void beam_fragment_collisions(int beam_id){
  for(int frag_id = 0; frag_id < MAX_FRAGMENTS; ++frag_id){
    if(fragment[frag_id].active){
      beam_fragment_collision(&beam[beam_id], &fragment[frag_id], frag_id, beam_id);
    }
  }
}

// checks collisions of beams with invaders, only if the beam is active and there are more than 0 invaders on screen
void check_beam_collisions(){

  for(int beam_id = 0; beam_id < MAX_BEAMS; ++beam_id){
    if(beam[beam_id].active){
      if(status.asteroids > 0){
        beam_asteroid_collisions(beam_id);
      }
      if(status.boulders > 0){
        beam_boulder_collisions(beam_id);
      }
      if(status.fragments > 0){
        beam_fragment_collisions(beam_id);
      }
    }
  }

}

// checks collision of the barrier only if there are more than 0 invaders on screeen
void check_barrier_collisions(){

  if(status.asteroids > 0){
    barrier_asteroid_collisions();
  }
  if(status.boulders > 0){
    barrier_boulder_collisions();
  }
  if(status.fragments > 0){
    barrier_fragment_collisions();
  }

}

// not a collision detection, but a collision event (when an invader hits the bottom of the screen) that restarts if allowed, or goes to game over mode
void invader_bottomscreen_collision(){
  if(status.lives > 1){
    --status.lives;
    setup();
  }

  else{
    --status.lives;
    send_str_P("\r\nGAME OVER\r\n");
    send_status();
    game_over();
  }
}

//
//  DRAW FUNCTIONS
//

void draw_starfighter() {
  draw_sprite(STARFIGHTER, round(starfighter.x), round(starfighter.y), 1);
}

// find the end (cx, cy) point of the cannon based on the starfighter position and potentiometer aim
void draw_cannon() {

  int startx = round(starfighter.x) + 4;
  int starty = round(starfighter.y);

  double endx_ = 3.0 * sin(status.aim * M_PI/180);
  double endy_ = 3.0 * cos(status.aim * M_PI/180);

  cx = startx + round(endx_);
  cy = starfighter.y - round(endy_);

  draw_line(startx, starty, cx, cy, 1);

}

void draw_beam(int i) {
  draw_sprite(BEAM, round(beam[i].x), round(beam[i].y), 1);
}

// draw beam only if it is active
void draw_beams(){

  for(int i = 0; i < MAX_BEAMS; ++i){
    if(beam[i].active){
      draw_beam(i);
    }
  }
}

void draw_asteroid(int i){
  draw_sprite(ASTEROID, round(asteroid[i].x), round(asteroid[i].y), 1);
}

void draw_asteroids() {

  for(int i = 0; i < MAX_ASTEROIDS; i++){
    if(asteroid[i].active){
      draw_asteroid(i);
    }
  }
}


void draw_boulder(int i){
  draw_sprite(BOULDER, round(boulder[i].x), round(boulder[i].y), 1);
}

void draw_boulders(){
  for(int i = 0; i < MAX_BOULDERS; ++i){
    if(boulder[i].active){
      draw_boulder(i);
    }
  }
}


void draw_fragment(int i){
  draw_sprite(FRAGMENT, round(fragment[i].x), round(fragment[i].y), 1);
}

void draw_fragments(){
  for(int i = 0; i < MAX_FRAGMENTS; ++i){
    if(fragment[i].active){
      draw_fragment(i);
    }
  }
}

//
//  UPDATE FUNCTIONS
//

// update asteroid based on position
void update_asteroid(int i) {

  asteroid[i].dy = status.speed * 0.001;
  asteroid[i].dx = 0;
  asteroid[i].y += asteroid[i].dy;

  if(asteroid[i].y + AST_SIZE > LCD_Y){
    invader_bottomscreen_collision();
  }
}

// warning lights flash for 2 secs before asteroids fall. left led if avg = left, right led if avg = right.
void asteroid_warning(){

  //get average asteroid position
  int avg_pos = (round(asteroid[0].x - 4) + round(asteroid[1].x - 4) + round(asteroid[2].x - 4)) / 3;

  //if avg = right
  if(avg_pos >= LCD_X/2){
    //flash at 2hz for 2 secs
    if(status.clock[2] < 11 && status.clock[2] > 5){
      LED1_ON; LED0_OFF;
    }

    else{
      LEDBOTH_OFF;
    }
  }

  // if avg = left
  else if(avg_pos < LCD_X/2){
    //flash at 2hz for 2 secs
    if(status.clock[2] < 11 && status.clock[2] > 5){
      LED0_ON; LED1_OFF;
    }

    else{
      LEDBOTH_OFF;
    }
  }

}

// warn that the asteroids are coming (2 seconds), and then update all asteroids
void update_asteroids(){

  if(status.time - status.start_time < 2){
    asteroid_warning();
  }

  else{
    LEDBOTH_OFF;
    for(int i = 0; i < 3; i++){
      update_asteroid(i);
    }
  }

}

// call if you want starfighter to go left
void starfighter_left(){
  starfighter.left = true;
  starfighter.right = false;
  starfighter.stationary = false;
}

//stationary
void starfighter_stop(){
  starfighter.left = false;
  starfighter.right = false;
  starfighter.stationary = true;
}

//right
void starfighter_right(){
  starfighter.left = false;
  starfighter.right = true;
  starfighter.stationary = false;
}

// update starfighter velocity based on joystick, velocity states and serial input
// -= 0.4 because 1 was too fast
void update_starfighter(char ch) {
  if(starfighter.left && round(starfighter.x) > 0){
    starfighter.x -= 0.4;
  }

  else if(starfighter.right && round(starfighter.x) < LCD_X - SF_WIDTH){
    starfighter.x += 0.4;
  }
}

// update boulder velocity based on position
void update_boulder(int i){

  //if it hits a wall, bounce off it
  if(boulder[i].x < 0 || boulder[i].x + BOL_SIZE > LCD_X){
    boulder[i].dx = -boulder[i].dx;
    boulder[i].y += (status.speed * 0.001) * boulder[i].dy;
  }

  //else update it normally
  else{
    boulder[i].x += (status.speed * 0.001) * boulder[i].dx;
    boulder[i].y += (status.speed * 0.001) * boulder[i].dy;
  }

  //if it hits the bottom of the screen, do bottomscreen collision
  if(boulder[i].y + BOL_SIZE > LCD_Y){
    invader_bottomscreen_collision();
  }
}

// update all boulders
void update_boulders(){
  for(int i = 0; i < 6; ++i){
    if(boulder[i].active){
      update_boulder(i);
    }
  }
}

//update fragment (same as update boulder but for a fragment now)
void update_fragment(int i){

  if(fragment[i].x < 0 || fragment[i].x + FRAG_SIZE > LCD_X){
    fragment[i].dx = -fragment[i].dx;
    fragment[i].x += (status.speed * 0.001) * fragment[i].dy;
  }

  else{
    fragment[i].x += (status.speed * 0.001) * fragment[i].dx;
    fragment[i].y += (status.speed * 0.001) * fragment[i].dy;
  }

  if(fragment[i].y + FRAG_SIZE > LCD_Y){
    invader_bottomscreen_collision();
  }
}

void update_fragments(){
  for(int i = 0; i < 12; ++i){
    if(fragment[i].active){
      update_fragment(i);
    }
  }
}

// the way conflict is dealt with from changing aim/speed in serial input was approved by Lawrence in the lecture
// if the explanation is confusing here, the test.txt file may help a bit more

// update aim of cannon based on pot0
// if a new aim has been enetered in serial input, the potentiometer needs to be set to +-5 of that value.
// e.g. if pot0 equates to 60 deg, and -30 deg is applied at serial input, the potentiometer won't change the aim until it is turned back to a value that equates to (-25 to -35 deg). Pot0 can now freely change the aim.
void update_cannon(){

  double angle = (input.pot0 - 128) * 0.46875; // convert potentiometer reading from (0->255) into (-60->60)

  // serial input conflict logic
  // if pot0 has been disabled due to a new serial input override, it needs to turn back to +-5 of that override angle
  if(!input.pot0_enabled && (status.aim < angle + 5 && status.aim > angle - 5)){
    input.pot0_enabled = true;
  }

  //if pot0 is enabled, update angle based on it
  else if(input.pot0_enabled){
    status.aim = round(angle);
  }

}

// update speed based on pot1
// this handles serial input override the same way as update_cannon()
void update_speed(){

  double speed = input.pot1 / 2; //speed (0->255) to (0->127), speed 256 was way too fast

  //serial input conflict logic
  if(!input.pot1_enabled && (status.speed < speed + 5 && status.speed > speed - 5)){
    input.pot1_enabled = true;
  }

  else if(input.pot1_enabled){
    status.speed = speed;
  }
}

// update beam based on position, clear it if it hits a wall
void update_beam(int i) {

  int new_x = round(beam[i].x + beam[i].dx);
  int new_y = round(beam[i].y - beam[i].dy);

  if(new_x < 0 || new_x > LCD_X - 2 || new_y < 0 || new_y > LCD_Y - 1){
    clear_beam(i);
  }

  else {
    beam[i].x += beam[i].dx;
    beam[i].y -= beam[i].dy;
  }

}

// update all beams only if active
void update_beams() {
  for(int i = 0; i < MAX_BEAMS; i++){
    update_beam(i);
  }
}

//
//  SERIAL I/O FUNCTIONS
//

// status screen to show on teensy lcd
void status_screen(){

  clear_screen();

  char score[3]; snprintf(score, sizeof(score), "%d", status.score);
  char lives[3]; snprintf(lives, sizeof(lives), "%d", status.lives);
  char time[6]; snprintf(time, sizeof(time), "%02d:%02d", status.clock[1], status.clock[0]);

  draw_string(1, 1, "time :", 1); draw_string(40, 1, time, 1);
  draw_string(1, 15, "lives:", 1); draw_string(40, 15, lives, 1);
  draw_string(1, 30, "score:", 1); draw_string(40, 30, score, 1);

  show_screen();

}

// status text to send to serial output
void send_status(){

  divider();
  char time[6]; snprintf(time, sizeof(time), "%02d:%02d", status.clock[1], status.clock[0]);
  send_str_P("time: "); send_str(time);
  send_str_P("\r\nlives: "); send_int(status.lives);
  send_str_P("\r\nscore: "); send_int(status.score);
  send_str_P("\r\nvisible asteroids: "); send_int(status.asteroids);
  send_str_P("\r\nvisible boulders: "); send_int(status.boulders);
  send_str_P("\r\nvisible fragments: "); send_int(status.fragments);
  send_str_P("\r\nvisible plasma: "); send_int(status.beams);
  send_str_P("\r\nturret aim: "); send_int(status.aim);
  send_str_P("\r\nspeed: "); send_int(status.speed);
  divider();

}

// get a new score from serial input from 0 to 999
void change_score(){

  divider();
  send_str_P("choose a new score {0,...,999}:\r\n");
  int new_score = receive_ints(3);
  if(new_score < 0 || new_score > 999){
    invalid_input();
    return;
  }
  status.score = new_score;
  send_str_P("\r\nscore set to: \r\n"); send_int(new_score);
  divider();
}

// change shield health
void change_shield(){

  divider();
  send_str_P("choose a new barrier health {0,...,5}:\r\n");

  int new_shield = receive_ints(1);
  if(new_shield < 0 || new_shield > 5){
    invalid_input();
    return;
  }
  status.shield = new_shield;
  send_str_P("\r\nshield health set to: "); send_int(new_shield);
  divider();
}

// change starfighter x pos
void change_starfighter(){

  divider();
  send_str_P("choose a new starfighter x position {0,...,75}:\r\n");

  int new_pos = receive_ints(2);
  if(new_pos < 0 || new_pos > 75){
    invalid_input();
    return;
  }
  starfighter_stop();
  starfighter.x = new_pos;
  send_str_P("\r\nx position set to: "); send_int(new_pos);
  divider();
}

//change cannon aim from serial input
void change_aim(){

  divider();
  send_str_P("choose a new aim (deg) {-60,...,60}:\r\n");

  int new_aim = receive_ints(2);
  if(new_aim < -60 || new_aim > 60){
    invalid_input();
    return;
  }

  //disable input from pot0 (not permanently)
  input.pot0_enabled = false;
  status.aim = new_aim;
  send_str_P("\r\naim set to: "); send_int(new_aim);
  divider();

}

// change speed of invaders
void change_speed(){

  divider();
  send_str_P("choose a new speed {0,...,127}:\r\n");

  int new_speed = receive_ints(3);
  if(new_speed < 0 || new_speed > 127){
    invalid_input();
    return;
  }
  input.pot1_enabled = false;
  status.speed = new_speed;
  send_str_P("\r\nnew speed set to: "); send_int(new_speed);
  divider();

}

// place an asteroid with serial input with no velocity
void place_asteroid(){

  //don't open this function if there are max asteroids already
  if(status.asteroids == MAX_ASTEROIDS) return;

  divider();
  send_str_P("place an asteroid: ");
  //find an inactive asteroid to overwrite
  int i = find_inactive_asteroid();
  ++status.asteroids;

  send_str_P("\r\nx position {0,...,78}: ");
  int x = receive_ints(2);
  if(x < 0 || x > 78){
    invalid_input();
    return;
  }
  asteroid[i].x = x;
  send_str_P("\r\nx set to: "); send_int(x);

  send_str_P("\r\ny position{0,...,42}: ");
  int y = receive_ints(2);
  if(y < 0 || y > 42){
    invalid_input();
    return;
  }
  asteroid[i].y = y;
  send_str_P("\r\ny set to: "); send_int(y);
  divider();

  asteroid[i].dx = 0;
  asteroid[i].dy = 0;
  asteroid[i].active = true;

}

void place_boulder(){

  if(status.boulders == MAX_BOULDERS) return;

  divider();
  send_str_P("place a boulder: ");

  int i = find_inactive_boulder();
  ++status.boulders;

  send_str_P("\r\nx position {0,...,79}: ");
  int x = receive_ints(2);
  if(x < 0 || x > 79){
    invalid_input();
    return;
  }

  boulder[i].x = x;
  send_str_P("\r\nx set to: "); send_int(x);

  send_str_P("\r\ny position{0,...,43}: ");
  int y = receive_ints(2);
  if(y < 0 || y > 43){
    invalid_input();
    return;
  }

  boulder[i].y = y;
  send_str_P("\r\ny set to: "); send_int(y);
  divider();

  boulder[i].dx = 0;
  boulder[i].dy = 0;
  boulder[i].active = true;

}

void place_fragment(){

  if(status.fragments == MAX_FRAGMENTS) return;

  divider();
  send_str_P("place a fragment: ");

  int i = find_inactive_fragment();
  ++status.fragments;

  send_str_P("\r\nx position {0,...,81}: ");
  int x = receive_ints(2);
  if(x < 0 || x > 81){
    invalid_input();
    return;
  }
  fragment[i].x = x;
  send_str_P("\r\nx set to: "); send_int(x);

  send_str_P("\r\ny position{0,...,45}: ");
  int y = receive_ints(2);
  if(y < 0 || y > 45){
    invalid_input();
    return;
  }
  fragment[i].y = y;
  send_str_P("\r\ny set to: "); send_int(y);
  divider();

  fragment[i].dx = 0;
  fragment[i].dy = 0;
  fragment[i].active = true;

}

//
//   MAIN FUNCTIONS
//

// function that calls functions/does stuff when input or serial input is pressed
void controls(char ch) {

  //fire a beam
  if(input.joystick_u == 1 || ch == 'w'){

  //ensures beams on screen are below maximum and the last beam fired was over 0.2 seconds ago
    if(status.beams < MAX_BEAMS &&  status.time - beam[status.latest_beam].fired_at > 0.2){
      replace_beam();
    }
  }

  // left joystick: starfighter movement
  else if((input.joystick_l == 1 || ch == 'a') && !input.lock_joystick_l){

    if(starfighter.stationary){
      starfighter_left();
    }

    else if(starfighter.right){
      starfighter_stop();
    }

    //lock input from joystick l until it is released to prevent it from being held for too long
    input.lock_joystick_l = true;

  }

  // right joystick: starfighter movement
  else if((input.joystick_r == 1 || ch == 'd') && !input.lock_joystick_r){

    if(starfighter.stationary){
      starfighter_right();
    }

    else if(starfighter.left){
      starfighter_stop();
    }

    //lock input from joystick r until it is released to prevent it from being held for too long
    input.lock_joystick_r = true;
  }

  // up joystick: pause
  else if((input.joystick_c == 1 || ch == 'p') && !input.lock_joystick_c){
    status.paused = !status.paused;
    //lock input from joystick c until it is released to prevent flickering between pause states
    input.lock_joystick_c = true;
  }

  // down joystick: status
  else if((input.joystick_d == 1 || ch == 's') && !input.lock_joystick_d){
    // toggle between shwoing or closing the status screen
    status.show = !status.show;
    if(!status.paused) send_status();
    else if(status.paused && status.show) send_status();
    // lock input from joystick d (same reason as above)
    input.lock_joystick_d = true;
  }

  else if(input.sw1 == 1 || ch == 'r'){
    setup();
  }

  else if(input.sw2 == 1 || ch == 'q'){
    quit();
  }

  // serial i/o
  else if(ch == 'l'){
    change_shield();
  }

  else if(ch == '?'){
    computer_help();
  }

  else if(ch == 'g'){
    change_score();
  }

  else if(ch == 'm'){
    change_speed();
  }

  else if(ch == 't'){
    change_aim();
  }

  else if(ch == 'h'){
    change_starfighter();
  }

  else if(ch == 'j'){
    place_asteroid();
  }

  else if(ch == 'k'){
    place_boulder();
  }

  else if(ch == 'i'){
    place_fragment();
  }
}

// clock function based on internal timer
// clock[2] is every 100ms (from 0->10), clock[0] is seconds (from 0->60), clock[1] is minutes (0->int limit). no hour timer
void clock(){

  int prev_time = floor(status.time * 10);
  status.time += TCNT0 * 2 * PRESCALE / FREQ;
  int new_time = floor(status.time * 10);

  if(new_time - prev_time == 1){
    ++status.clock[2];
  }

  if(status.clock[2] == 10){
    status.clock[2] -= 10;
    ++status.clock[0];
  }

  if(status.clock[0] == 60){
    status.clock[0] -= 60;
    ++status.clock[1];
  }

  // sorry
  if(status.clock[1] == 10 && status.clock[0] == 0 && status.clock[2] == 0){
    send_str_P("bruh how has this level been running for 10 minutes");
  }

}

// update everything based on status values and controls
// only update things if they are active for better efficiency
void update_all(char ch){

  update_starfighter(ch);
  update_cannon();
  update_speed();

  if(status.shield > 0){
    check_barrier_collisions();
  }

  if(status.beams > 0){
    check_beam_collisions();
    update_beams();
  }

  if(status.asteroids > 0){
    update_asteroids();
  }

  if(status.boulders > 0){
    update_boulders();
  }

  if(status.fragments > 0){
    update_fragments();
  }

  // if all invaders are defetead, start a new game
  if(status.fragments == 0 && status.asteroids == 0 && status.boulders == 0){
    setup();
  }
}

// draw everything only if they are active
void draw_all(){

  if(status.asteroids > 0){
    draw_asteroids();
  }

  if(status.boulders > 0){
    draw_boulders();
  }

  if(status.fragments > 0){
    draw_fragments();
  }

  if(status.beams > 0){
    draw_beams();
  }

  if(status.shield > 0){
    draw_barrier();
  }

  draw_starfighter();
  draw_cannon();

}

// main loop function
void loop() {

  // get serial input
  int16_t char_code = usb_serial_getchar();
  char ch = char_code;

  // get potentiometer values every loop
  input.pot0 = read_pot(0);
  input.pot1 = read_pot(1);

  // call controls function every loop
  controls(ch);

  // when the game is first resumed (when a new game is released from pause mode), send stuff to terminal (only once)
  if(status.first_resume && status.paused){
    divider();
    send_str_P("GAME STARTED\r\n");
    send_status();
    status.first_resume = false;
  }

  // if game isn't paused, update and draw everything
  if(!status.paused){
    clock();
    update_all(ch);
    clear_screen();
    draw_all();
    show_screen();
  }

  // if game is paused and status wants to be shown, show status
  else if(status.show && status.paused){
    status_screen();
  }

  // if game is paused, draw "paused" on screen and draw eveything without updating anyhting
  else if(status.paused){
    clear_screen();
    draw_string(27, 30, "paused", 1);
    draw_all();
    show_screen();
  }

}

int main() {

  // setup to only run when teensy is first powered on
  init_setup();

  while(1){
    loop();
  }

  return 0;
}
