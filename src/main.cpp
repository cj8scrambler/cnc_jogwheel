#include <cstdio>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/irq.h"

#define ADC_MAX      (1 << 12)

#define JOY_LEFT            6
#define JOY_DOWN            7
#define JOY_RIGHT           8
#define JOY_UP              9

#define ROT_A              10
#define ROT_B              11

#define BTN_XY_ZERO        12
#define BTN_Z_ZERO         13
#define BTN_HOME           14
#define BTN_EXTRA          15

#define AIN_XY_SPEED_PIN   31
#define AIN_XY_SPEED_CHAN   0
#define AIN_Z_SPEED_PIN    32
#define AIN_Z_SPEED_CHAN    1

volatile uint8_t joy_up=0;
volatile uint8_t joy_right=0;
volatile uint8_t joy_down=0;
volatile uint8_t joy_left=0;
volatile bool joy_poll=false;

volatile uint8_t btn_xy_zero=0;
volatile uint8_t btn_z_zero=0;
volatile uint8_t btn_home=0;
volatile uint8_t btn_extra=0;

volatile bool event_xy_speed=0;
volatile uint8_t xy_speed=0;
volatile bool event_z_speed=0;
volatile uint8_t z_speed=0;

volatile bool event_encoder = false;
volatile uint16_t encoder_position = 0;
uint8_t last_encoder_state = 0;

void handle_encoder(void)
{
  int8_t rotary_lookup[4][4] = { { 0, -1,  1,  2},
                                 { 1,  0,  2, -1},
                                 {-1,  2,  0,  1},
                                 { 2,  1, -1,  0} };

  uint8_t current_encoder_state = (gpio_get(ROT_A) << 1) | gpio_get(ROT_B);
  encoder_position += rotary_lookup[last_encoder_state][current_encoder_state];
  last_encoder_state = current_encoder_state;
  event_encoder = true;
}

void gpio_isr(uint gpio, uint32_t events)
{

  // TODO: Debounce buttons
  switch (gpio) {
  case JOY_LEFT:
    if (events & GPIO_IRQ_EDGE_FALL) {
      joy_left = 1;
      joy_poll = true;
    }
    break;
  case JOY_UP:
    if (events & GPIO_IRQ_EDGE_FALL) {
      joy_up = 1;
      joy_poll = true;
    }
    break;
  case JOY_RIGHT:
    if (events & GPIO_IRQ_EDGE_FALL) {
      joy_right = 1;
      joy_poll = true;
    }
    break;
  case JOY_DOWN:
    if (events & GPIO_IRQ_EDGE_FALL) {
      joy_down = 1;
      joy_poll = true;
    }
    break;
  case BTN_XY_ZERO:
    if (events & GPIO_IRQ_EDGE_FALL) {
      btn_xy_zero = 1;
    }
    break;
  case BTN_Z_ZERO:
    if (events & GPIO_IRQ_EDGE_FALL) {
      btn_z_zero = 1;
    }
    break;
  case BTN_HOME:
    if (events & GPIO_IRQ_EDGE_FALL) {
      btn_home = 1;
    }
    break;
  case BTN_EXTRA:
    if (events & GPIO_IRQ_EDGE_FALL) {
      btn_extra = 1;
    }
    break;
  case ROT_A:
  case ROT_B:
    handle_encoder();
    break;
  }
}

void gpio_init(void)
{

    gpio_init(JOY_LEFT);
    gpio_set_dir(JOY_LEFT, GPIO_IN);
    gpio_pull_up(JOY_LEFT);
    gpio_set_irq_enabled(JOY_LEFT, GPIO_IRQ_EDGE_FALL, true);

    gpio_init(JOY_DOWN);
    gpio_set_dir(JOY_DOWN, GPIO_IN);
    gpio_pull_up(JOY_DOWN);
    gpio_set_irq_enabled(JOY_DOWN, GPIO_IRQ_EDGE_FALL, true);

    gpio_init(JOY_RIGHT);
    gpio_set_dir(JOY_RIGHT, GPIO_IN);
    gpio_pull_up(JOY_RIGHT);
    gpio_set_irq_enabled(JOY_RIGHT, GPIO_IRQ_EDGE_FALL, true);

    gpio_init(JOY_UP);
    gpio_set_dir(JOY_UP, GPIO_IN);
    gpio_pull_up(JOY_UP);
    gpio_set_irq_enabled(JOY_UP, GPIO_IRQ_EDGE_FALL, true);

    gpio_init(ROT_A);
    gpio_set_dir(ROT_A, GPIO_IN);
    gpio_set_irq_enabled(ROT_A, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    gpio_init(ROT_B);
    gpio_set_dir(ROT_B, GPIO_IN);
    gpio_set_irq_enabled(ROT_B, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    gpio_init(BTN_XY_ZERO);
    gpio_set_dir(BTN_XY_ZERO, GPIO_IN);
    gpio_pull_up(BTN_XY_ZERO);
    gpio_set_irq_enabled(BTN_XY_ZERO, GPIO_IRQ_EDGE_FALL, true);

    gpio_init(BTN_Z_ZERO);
    gpio_set_dir(BTN_Z_ZERO, GPIO_IN);
    gpio_pull_up(BTN_Z_ZERO);
    gpio_set_irq_enabled(BTN_Z_ZERO, GPIO_IRQ_EDGE_FALL, true);

    gpio_init(BTN_HOME);
    gpio_set_dir(BTN_HOME, GPIO_IN);
    gpio_pull_up(BTN_HOME);
    gpio_set_irq_enabled(BTN_HOME, GPIO_IRQ_EDGE_FALL, true);

    gpio_init(BTN_EXTRA);
    gpio_set_dir(BTN_EXTRA, GPIO_IN);
    gpio_pull_up(BTN_EXTRA);
    gpio_set_irq_enabled(BTN_EXTRA, GPIO_IRQ_EDGE_FALL, true);

    /* One ISR for all GPIOs */
    gpio_set_irq_callback(&gpio_isr);

    adc_init();
    adc_gpio_init(AIN_XY_SPEED_PIN); 
    adc_gpio_init(AIN_Z_SPEED_PIN); 
}

void poll_joystick(void)
{
  bool any_active = false;
  if (!gpio_get(JOY_LEFT)) {
    joy_left = 1;
    any_active = true;
  }
  if (!gpio_get(JOY_UP)) {
    joy_up = 1;
    any_active = true;
  }
  if (!gpio_get(JOY_RIGHT)) {
    joy_right = 1;
    any_active = true;
  }
  if (!gpio_get(JOY_DOWN)) {
    joy_down = 1;
    any_active = true;
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
  if (joy_left || joy_up || joy_right || joy_down) {
    printf("Joystick:");
    if (joy_left) {
      printf(" LEFT ");
      joy_left = 0;
    }
    if (joy_up) {
      printf("  UP  ");
      joy_up = 0;
    }
    if (joy_right) {
      printf(" RIGHT");
      joy_right = 0;
    }
    if (joy_down) {
      printf(" DOWN ");
      joy_down = 0;
    }
    printf("\n");
  }
}

void handle_button(void)
{
  if (btn_xy_zero) {
    printf(" Button: XY-Zero\n");
    btn_xy_zero = 0;
  }
  if (btn_z_zero) {
    printf(" Button: Z-Zero\n");
    btn_z_zero = 0;
  }
  if (btn_home) {
    printf(" Button: Home\n");
    btn_home = 0;
  }
  if (btn_extra) {
    printf(" Button: Extra\n");
    btn_extra = 0;
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

int main() {
    stdio_init_all();
    gpio_init();
printf("Begin\n");

    while (true) {
      if (check_ain(AIN_XY_SPEED_CHAN) || check_ain(AIN_XY_SPEED_CHAN))
        handle_ain();

      if (joy_poll)
        poll_joystick();

      handle_rotary();
      handle_joy();
      handle_button();

//      tight_loop_contents(); // Keep the main loop running
    }

    return 0;
}
