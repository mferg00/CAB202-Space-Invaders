//values for starfighter
typedef struct starfighter_data{
  double x;
  double y;
  bool stationary;
  bool left;
  bool right;
}starfighter_data;

//values for each beam
typedef struct beam_data{
  double dir;
  double x;
  double y;
  double dx;
  double dy;
  double fired_at;
  bool active;
}beam_data;

//values for each invader
typedef struct invader_data{
  double x;
  double y;
  double dir;
  double dy;
  double dx;
  bool active;
}invader_data;

//values for important data
typedef struct status_data{
  double start_time;
  double time;
  int clock[3];
  int lives;
  int score;
  int asteroids;
  int boulders;
  int fragments;
  int beams;
  int latest_beam;
  int aim;
  int shield;
  double speed;
  bool paused;
  bool show;
  bool first_resume;
}status_data;

//values for inputs
typedef volatile struct input_data{

  uint8_t pot0;
  uint8_t pot1;
  uint8_t joystick_l;
  uint8_t joystick_r;
  uint8_t joystick_u;
  uint8_t joystick_d;
  uint8_t joystick_c;
  uint8_t sw1;
  uint8_t sw2;
  bool pot0_enabled;
  bool pot1_enabled;
  bool lock_joystick_c;
  bool lock_joystick_d;
  bool lock_joystick_r;
  bool lock_joystick_l;

}input_data;
