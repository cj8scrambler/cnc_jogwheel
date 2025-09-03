#define AIN_XY_SPEED_CHAN   0
#define AIN_Z_SPEED_CHAN    1

void jogwheel_io_init(void);

void poll_joystick(void);
bool check_ain(int chan);

void handle_encoder(void);
void handle_rotary(void);
void handle_joy(void);
void handle_button(void);
void handle_ain(void);

void set_led(bool enable);
