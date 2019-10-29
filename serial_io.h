#define send_str_P(x) (send_str_progmem(PSTR(x)))   //shortcut to store a string in PROGMEM (to save precious RAM) and send it to usb serial out. void send_str_PROGMEM() is defined in serial_io.c

void send_str_progmem(const char *str);
//stolen from topic 10 usb_serial_echo.c
void send_str(char * message);

//these two only work for small amounts of digits, which is fine for this project
void send_double(double num);

void send_int(int num);

// maximum 3 digit long int from the console. first value is the sign. eg typing 123 will return {1, 1,2,3}, and typing -123 will return {-1, 1,2,3}.
//the values received from the console need to be converted from: 16bit char -> char -> int
int receive_ints(int num_digits);

void divider();
// create a small function to rpevent storing multiple copies of the same string in PROGMEM
void invalid_input();

// send the controls (PROGMEM const string) to serial output
void computer_help();
