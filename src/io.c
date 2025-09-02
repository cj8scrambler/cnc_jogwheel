#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/irq.h"

#include "io.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define ADC_MAX      (1 << 12)

#define JOY_LEFT_GPIO       6
#define JOY_DOWN_GPIO       7
#define JOY_RIGHT_GPIO      8
#define JOY_UP_GPIO         9

#define BTN_XY_ZERO_GPIO   12
#define BTN_Z_ZERO_GPIO    13
#define BTN_HOME_GPIO      14
#define BTN_EXTRA_GPIO     15

#define ROT_A_GPIO         10
#define ROT_B_GPIO         11

#define AIN_XY_SPEED_PIN   31
//#define AIN_XY_SPEED_CHAN   0
#define AIN_Z_SPEED_PIN    32
//#define AIN_Z_SPEED_CHAN    1

#define MAX_BTN_NAME_LEN   16
#define BTN_DEBOUNCE_MS    50

typedef enum btn {
  JOY_LEFT,    /* Code below assumes JOY buttons are in order L-U-R-D */
  JOY_UP,
  JOY_RIGHT,
  JOY_DOWN,
  BTN_XY_ZERO,
  BTN_Z_ZERO,
  BTN_HOME,
  BTN_EXTRA,

  NUM_BTNS, /* leave last */
} btn_t;

typedef enum btn_state {
  IDLE = 0x0,
  ACTIVE,
  DEBOUNCING,
} btn_state_t;

typedef struct btn_info {
  btn_state_t state;
  int gpio;
  const char name[MAX_BTN_NAME_LEN];
} btn_info_t;

/* Button info table; also need to add to gpio_to_button() */
volatile btn_info_t button_info[] = {
  [JOY_LEFT] = {IDLE, JOY_LEFT_GPIO, "Joy-Left"},
  [JOY_UP] = {IDLE, JOY_UP_GPIO, "Joy-Up"},
  [JOY_RIGHT] = {IDLE, JOY_RIGHT_GPIO, "Joy-Right"},
  [JOY_DOWN] = {IDLE, JOY_DOWN_GPIO, "Joy-Down"},
  [BTN_XY_ZERO] = {IDLE, BTN_XY_ZERO_GPIO, "Button-XY-Zero"},
  [BTN_Z_ZERO] = {IDLE, BTN_Z_ZERO_GPIO, "Button-Z-Zero"},
  [BTN_HOME] = {IDLE, BTN_HOME_GPIO, "Button-Home"},
  [BTN_EXTRA] = {IDLE, BTN_EXTRA_GPIO, "Button-Extra"},
};

volatile bool joy_poll = false;

volatile uint8_t xy_speed = 0;
volatile bool event_xy_speed = 0;
volatile uint8_t z_speed = 0;
volatile bool event_z_speed = 0;

volatile uint16_t encoder_position = 0;
uint8_t last_encoder_state = 0;
volatile bool event_encoder = false;

void handle_encoder(void)
{
  int8_t rotary_lookup[4][4] = { { 0, -1,  1,  2},
                                 { 1,  0,  2, -1},
                                 {-1,  2,  0,  1},
                                 { 2,  1, -1,  0}};

  uint8_t current_encoder_state = (gpio_get(ROT_A_GPIO) << 1) | gpio_get(ROT_B_GPIO);
  encoder_position += rotary_lookup[last_encoder_state][current_encoder_state];
  last_encoder_state = current_encoder_state;
  event_encoder = true;
}

btn_t gpio_to_button(uint gpio)
{
  switch(gpio) {
  case JOY_LEFT_GPIO:
    return JOY_LEFT;
  case JOY_DOWN_GPIO:
    return JOY_DOWN;
  case JOY_RIGHT_GPIO:
    return JOY_RIGHT;
  case JOY_UP_GPIO:
    return JOY_UP;
  case BTN_XY_ZERO_GPIO:
    return BTN_XY_ZERO;
  case BTN_Z_ZERO_GPIO:
    return BTN_Z_ZERO;
  case BTN_HOME_GPIO:
    return BTN_HOME;
  case BTN_EXTRA_GPIO:
    return BTN_EXTRA;
  }

  printf("Error: Unhandled GPIO: %d\n", gpio);
  return NUM_BTNS;
}

int64_t debounce_handler(alarm_id_t id, void *user_data)
{
  volatile btn_info_t *button = (btn_info_t *)user_data;

  /* If it's still low, then it's good; otherwise back to idle */
  if (gpio_get(button->gpio)) {
      button->state = IDLE;
  } else {
      button->state = ACTIVE;
printf("DZ: debounced button: %s\n", button->name);
  }
  return 0;
}

void gpio_isr(uint gpio, uint32_t events)
{
  btn_t button = gpio_to_button(gpio);

  switch (gpio) {
  case JOY_LEFT_GPIO:
  case JOY_UP_GPIO:
  case JOY_RIGHT_GPIO:
  case JOY_DOWN_GPIO:
  case BTN_XY_ZERO_GPIO:
  case BTN_Z_ZERO_GPIO:
  case BTN_HOME_GPIO:
  case BTN_EXTRA_GPIO:
    if ((events & GPIO_IRQ_EDGE_FALL) && (button_info[button].state = IDLE)) {
      button_info[button].state = DEBOUNCING;
      alarm_id_t id = add_alarm_at(
                        delayed_by_ms(get_absolute_time(), BTN_DEBOUNCE_MS),
                        debounce_handler, (void *)&button_info[button], true);           
    }
    break;

  case ROT_A_GPIO:
  case ROT_B_GPIO:
    handle_encoder();
    break;
  }
}

void jogwheel_io_init(void)
{
  for(int i = 0; i < ARRAY_SIZE(button_info); i++) {
    gpio_init(button_info[i].gpio);
    gpio_set_dir(button_info[i].gpio, GPIO_IN);
    gpio_pull_up(button_info[i].gpio);
    gpio_set_irq_enabled(button_info[i].gpio, GPIO_IRQ_EDGE_FALL, true);
  }

  gpio_init(ROT_A_GPIO);
  gpio_set_dir(ROT_A_GPIO, GPIO_IN);
  gpio_set_irq_enabled(ROT_A_GPIO, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

  gpio_init(ROT_B_GPIO);
  gpio_set_dir(ROT_B_GPIO, GPIO_IN);
  gpio_set_irq_enabled(ROT_B_GPIO, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

  /* One ISR for all GPIOs */
  gpio_set_irq_callback(&gpio_isr);

  adc_init();
  adc_gpio_init(AIN_XY_SPEED_PIN); 
  adc_gpio_init(AIN_Z_SPEED_PIN); 
}

void poll_joystick(void)
{
  bool any_active = false;
 
  for (btn_t button = JOY_LEFT; button <= JOY_DOWN; button++ ) {
    if (!gpio_get(button_info[button].gpio)) {
      button_info[button].state = ACTIVE;
      any_active = true;
    }
  }

  /* Stop polling none are active */
  if (!any_active) {
    joy_poll = false;
  }
}

bool check_ain(int chan)
{
  uint16_t scaled = ADC_MAX;
  uint16_t val;

  adc_select_input(chan);
  val = adc_read();

  if (val <= (ADC_MAX / 4))
    scaled = 0;
  else if (val > ((ADC_MAX * 3) / 4))
    scaled = 1;
  else
    scaled = 2;

  if ((chan == AIN_XY_SPEED_CHAN) && (scaled != xy_speed)) {
    xy_speed = scaled;
    event_xy_speed = 1;
    return true;
  }

  if ((chan == AIN_Z_SPEED_CHAN) && (scaled != z_speed)) {
    z_speed = scaled;
    event_z_speed = 1;
    return true;
  }

  return false;
}

void handle_rotary(void)
{
  if (event_encoder) {
    printf("  Rotary: %d\n", encoder_position);
    event_encoder = false;
  }
}

void handle_joy(void)
{
  if ((button_info[JOY_LEFT].state == ACTIVE) ||
      (button_info[JOY_UP].state == ACTIVE) ||
      (button_info[JOY_RIGHT].state == ACTIVE) ||
      (button_info[JOY_DOWN].state == ACTIVE)) {
    printf("Joystick:");
    if (button_info[JOY_LEFT].state == ACTIVE) {
      printf(" LEFT ");
      button_info[JOY_LEFT].state == IDLE;
    }
    if (button_info[JOY_UP].state == ACTIVE) {
      printf("  UP  ");
      button_info[JOY_UP].state == IDLE;
    }
    if (button_info[JOY_RIGHT].state == ACTIVE) {
      printf(" RIGHT");
      button_info[JOY_RIGHT].state == IDLE;
    }
    if (button_info[JOY_DOWN].state == ACTIVE) {
      printf(" DOWN ");
      button_info[JOY_DOWN].state == IDLE;
    }
    printf("\n");
  }
}

void handle_button(void)
{
  if (button_info[BTN_XY_ZERO].state == ACTIVE) {
    printf(" Button: XY-Zero\n");
    button_info[BTN_XY_ZERO].state == IDLE;
  }
  if (button_info[BTN_Z_ZERO].state == ACTIVE) {
    printf(" Button: Z-Zero\n");
    button_info[BTN_Z_ZERO].state == IDLE;
  }
  if (button_info[BTN_HOME].state == ACTIVE) {
    printf(" Button: Home\n");
    button_info[BTN_HOME].state == IDLE;
  }
  if (button_info[BTN_EXTRA].state == ACTIVE) {
    printf(" Button: Extra\n");
    button_info[BTN_EXTRA].state == IDLE;
  }
}

void handle_ain(void)
{
  if (event_xy_speed) {
    printf(" Switch: XY-Speed = %d\n", xy_speed);
    event_xy_speed = false;
  }
  if (event_z_speed) {
    printf(" Switch: Z-Speed = %d\n", z_speed);
    event_z_speed = false;
  }
}
