#include <stdio.h>
#include <avr/io.h>
#include "define.h"
#include "macros.h"
#include "hardware.h"

// volatile vars for inputs
volatile uint8_t bit_count[7] = {0};
volatile uint8_t pin_value[7];

// set backlight (0 = max, 1023 = min). Taken from adc_pwm_backlight.c from topic 11
void set_backlight(int duty_cycle) {

	// Reading PWM 10 bit value (we have to use two registers)
	// (a)	Set bits 8 and 9 of Output Compare Register 4A.
	TC4H = duty_cycle >> 8;

	// (b)	Set bits 0..7 of Output Compare Register 4A.
	OCR4A = duty_cycle & 0xff;
}

// setup potentiometer registers and etc
void setup_pot(){
  // set up potentiometers
  // use internal 2.56V ref, left-adjust result
  ADMUX |= (1<<REFS0)|(1<<REFS1)|(1<<ADLAR);
  // enable adc, 128 pre-scalar
  ADCSRA |= (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
  // free-running mode
  ADCSRB = 0;
}

// setup timer0 with 1024 prescale
void setup_timer0(){
  TCCR0A = 0;
  TCCR0B = 0b00000101;
  TIMSK0 = 0b00000001;
}

// setup timer4 for use with pwm of backlight. Taken from adc_pwm_backlight.c from topic 11
void setup_pwm(){

  TC4H = 1023 >> 8;
  OCR4C = 1023 & 0xff;
  // Enable PWM
  // (b)	Use OC4A for PWM. Remember to set DDR for C7.
  TCCR4A |= BIT(COM4A1) | BIT(PWM4A);

  // Bit set for the LCD backlight
  SET_BIT(DDRC, 7);

  // Topic 9 - Timer prescalar
  // (c)	Set pre-scale to "no pre-scale"
  TCCR4B |= BIT(CS42) | BIT(CS41) | BIT(CS40);

  // (c.1) If you must adjust TCCR4C, be surgical. If you clear
  //		bits COM4A1 and COM4A0 you will _disable_ PWM on OC4A,
  //		because the COM4Ax and COM4Bx hardware pins appear in
  //		both TCCR4A and TCCR4C!

  /* In this example, TCCR4C is not needed */

  // (d)	Select Fast PWM
  TCCR4D = 0;
}

void setup_led(){
  //enable led for output
  SET_BIT(DDRB, 2);
  SET_BIT(DDRB, 3);
}

// store 4 consecutive pin values in array
void input_history(uint8_t mask) {

  pin_value[0] = BIT_VALUE(PINB, 1);
  pin_value[1] = BIT_VALUE(PIND, 0);
  pin_value[2] = BIT_VALUE(PIND, 1);
  pin_value[3] = BIT_VALUE(PINB, 7);
  pin_value[4] = BIT_VALUE(PINB, 0);
  pin_value[5] = BIT_VALUE(PINF, 6);
  pin_value[6] = BIT_VALUE(PINF, 5);

  for(int i = 0; i < 7; i++){
    bit_count[i] = bit_count[i] << 1;
    bit_count[i] = bit_count[i] & mask;
    bit_count[i] = (bit_count[i] & mask) | pin_value[i];
  }
}

// for use in main to check if input is true or not
bool input_check(int bit, uint8_t mask){
  if(bit_count[bit] == mask){
    return true;
  }
  return false;
}

// read potentiometer 0 or 1
int read_pot(int id) {

  switch(id){
    case 0: ADMUX &= ~(1<<MUX0); break;
    case 1: ADMUX |= (1<<MUX0); break;
  }

  // start conversion
  ADCSRA |= (1<<ADSC);

  // wait until conversion is complete
  while(~ADCSRA&(1<<ADIF)){}

  // return the value from the adc
  return ADCH;
}
