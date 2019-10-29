#include <stdio.h>
#include <string.h>
#include <avr/pgmspace.h>
#include "usb_serial.h"
#include "serial_io.h"

// send a variable of type PSTR() to usb serial output. Taken from example.c in /usb_serial
// This is really important to do as there is a lot of const strings for the cheat functions and it takes up precious ram (could only get 5 beams maximum without this)
void send_str_progmem(const char *str)
{
	char c;
	while (1) {
		c = pgm_read_byte(str++);
		if (!c) break;
		usb_serial_putchar(c);
	}
}

//stolen from topic 10 usb_serial_echo.c
void send_str(char * message) {
	// Cast to avoid "error: pointer targets in passing argument 1
	//	of 'usb_serial_write' differ in signedness"
	usb_serial_write((uint8_t *) message, strlen(message));
}

//these two only work for small amounts of digits, which is fine for this project
void send_double(double num){
  char buf[8];
  snprintf(buf, sizeof(buf), "%g", num);
  send_str(buf);
}

void send_int(int num){
  char buf[4];
  snprintf(buf, sizeof(buf), "%d", num);
  send_str(buf);
}

// maximum 3 digit long int from the console. first value is the sign. eg typing 123 will return {1, 1,2,3}, and typing -123 will return {-1, 1,2,3}.
int receive_ints(int num_digits) {

  //first value is the sign (- or +), last three values are the numbers to be combined
  int serial_input[4] = {0};

  //input error checking
  if(num_digits > 3 || num_digits < 0){
    num_digits = 3;
  }

  //location in array serial_input
  int count = 0;

  //sign is not considered when deciding the number of digits, so add space for it (if user wants 999, they need '4' digits because it will be +1,999)
  num_digits = num_digits + 1;

  //only allow for -[num_digits] to +[num_digits]
  while(count < num_digits){

    int16_t char_code = usb_serial_getchar();

    //get the sign. user inputs '-' if negative, or they input their first number and '+' is assumed.
    if(count == 0){
				//char code 45 is '-'
			  if(char_code == 45 && char_code != -1){
        serial_input[count] = -1;
        count = 1;
        send_str_P("-");
      }
			//char codes 48 to 57 are numbers 0 to 9, so only accept them
      else if(char_code >= 48 && char_code <= 57 && char_code != -1){
        serial_input[count] = 1;
        send_str_P("+");
        char ascii_num = char_code; int num_received = ascii_num - 48; //convert ascii value for number into actual number
        serial_input[count + 1] = num_received;
        send_int(serial_input[count + 1]);
        count = 2;
      }
    }

    //get the digits. only allow ascii representing 0 to 9
    else if(char_code >= 48 && char_code <= 57 && char_code != -1) {
      char ascii_num = char_code; int num_received = ascii_num - 48;
      serial_input[count] = num_received;
      send_int(serial_input[count]);
      count++;
    }

    //if user presses enter, fill the rest of the array with a value to be ignored
    else if(char_code == '\r' || char_code == '\n'){
      for(int i = count; count < num_digits; i++){
        serial_input[i] = -10;
        count++;
      }
    }
  }

  //initialise int that will become the combination of the digits
  int k = 0;

  //combine digits together given that each digit is an increasing power of 10
  for (int i = 1; i < num_digits; i++){
    if(serial_input[i] != -10){
      k = 10 * k + serial_input[i];
    }
  }

  //return the sign * the full integer
  return k * serial_input[0];
}

// visual divider so serial output is less cluttered (stored in PROGMEM)
void divider(){
  send_str_P("\r\n==============================================================================\r\n");
}

// invalid input string to be used for cheat functions (PROGMEM)
void invalid_input(){
  send_str_P("\r\ninvalid input");
  divider();
}

// send the controls (PROGMEM) to serial output
void computer_help(){

  divider();
  send_str_P("a: move spaceship left\r\nd: move spaceship right\r\nw: fire plasma bolts\r\ns: send and display game status\r\nr: start or reset game\r\np: pause game\r\nq: quit\r\nt: set aim of the turret\r\nm: set the speed of the game\r\nl: set the remaining useful life of the deflector shield\r\ng: set the score\r\nh: move spaceship to coordinate\r\nj: place asteroid at coordinate\r\nk: place boulder at coordinate\r\nl: place fragment at coordinate");
  divider();

}
