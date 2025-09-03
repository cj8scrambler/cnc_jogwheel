#define AIN_XY_SPEED_CHAN   0
#define AIN_Z_SPEED_CHAN    1

void jogwheel_io_init(void);

// TODO: move to timer
bool check_ain(int chan);

void handle_inputs(void);

void set_led(bool enable);
