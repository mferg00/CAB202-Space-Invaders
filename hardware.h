//led toggle macros
#define LED1_ON SET_BIT(PORTB, 3)
#define LED1_OFF CLEAR_BIT(PORTB, 3)
#define LED0_ON SET_BIT(PORTB, 2)
#define LED0_OFF CLEAR_BIT(PORTB, 2)
#define LEDBOTH_OFF CLEAR_BIT(PORTB, 2); CLEAR_BIT(PORTB, 3)

// set backlight (0 = max, 1023 = min). Taken from adc_pwm_backlight.c from topic 11
void set_backlight(int duty_cycle);
// setup potentiometer registers and etc
void setup_pot();

// setup timer0 with 1024 prescale
void setup_timer0();

// setup timer4 for use with pwm of backlight. Taken from adc_pwm_backlight.c from topic 11
void setup_pwm();

void setup_led();

// store 4 consecutive pin values in array
void input_history(uint8_t mask);

bool input_check(int bit, uint8_t mask);
// read potentiometer 0 or 1
int read_pot(int id);
