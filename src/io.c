#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/irq.h"
#include "hardware/uart.h"

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

#include "grbl.h"
#include "io.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define ADC_MAX          (1 << 12)

#define GRBL_UART_BAUD     115200

/*      NAME                  GPIO      PIN  */
#define UART_TX_GRBL_RX         4    // 6
#define UART_RX_GRBL_TX         5    // 7
#if 0
#define JOY_LEFT_GPIO           6    // 9
#define JOY_DOWN_GPIO           7    // 10
#define JOY_RIGHT_GPIO          8    // 11
#define JOY_UP_GPIO             9    // 12
#else
#define JOY_LEFT_GPIO           8
#define JOY_DOWN_GPIO           7
#define JOY_RIGHT_GPIO          9
#define JOY_UP_GPIO             6
#endif

#define BTN_XY_ZERO_GPIO       12    // 16
#define BTN_Z_ZERO_GPIO        13    // 17
#define BTN_HOME_GPIO          14    // 18
#define BTN_PROBE_Z_GPIO       15    // 19
#define BTN_PROBE_XYZ_GPIO     16    // 21

#define ROT_A_GPIO             10    // 14
#define ROT_B_GPIO             11    // 15

#define AIN_XY_SPEED_PIN       26    // 31
#define AIN_XY_SPEED_CHAN       0    // ADC0
#define AIN_Z_SPEED_PIN        27    // 32
#define AIN_Z_SPEED_CHAN        1    // ADC1

#define MAX_BTN_NAME_LEN       16
#define BTN_DEBOUNCE_MS        50 /* button debounce time */
#define AIN_POLL_PERIOD_MS    100 /* how often to poll analog input switches */
#define BTN_AUTOREPEAT_MS     150 /* button press auto-repeat delay */

typedef enum btn {
  JOY_LEFT,    /* Code below assumes JOY buttons are in order L-U-R-D */
  JOY_UP,
  JOY_RIGHT,
  JOY_DOWN,
  BTN_XY_ZERO,
  BTN_Z_ZERO,
  BTN_HOME,
  BTN_PROBE_Z,
  BTN_PROBE_XYZ,

  NUM_BTNS, /* leave last */
} btn_t;

typedef struct btn_info {
  int gpio;
  bool debouncing;
  bool poll;
  uint16_t count;
  absolute_time_t repeat_time;
  const char name[MAX_BTN_NAME_LEN];
} btn_info_t;

#define BUTTON_INFO(gpio_num, name_str) \
  {.gpio = (gpio_num),       \
   .debouncing = false,  \
   .poll = false,        \
   .count = 0,           \
   .repeat_time = 0,     \
   .name = (name_str)}

/*
 * Button info table
 * All entries also need to be handled in gpio_to_button_info() as well
 */
volatile btn_info_t button_info[] = {
  [JOY_LEFT] =    BUTTON_INFO(JOY_LEFT_GPIO, "Joy-Left"),
  [JOY_UP] =      BUTTON_INFO(JOY_UP_GPIO, "Joy-Up"),
  [JOY_RIGHT] =   BUTTON_INFO(JOY_RIGHT_GPIO, "Joy-Right"),
  [JOY_DOWN] =    BUTTON_INFO(JOY_DOWN_GPIO, "Joy-Down"),
  [BTN_XY_ZERO] = BUTTON_INFO(BTN_XY_ZERO_GPIO, "Button-XY-Zero"),
  [BTN_Z_ZERO] =  BUTTON_INFO(BTN_Z_ZERO_GPIO, "Button-Z-Zero"),
  [BTN_HOME] =    BUTTON_INFO(BTN_HOME_GPIO, "Button-Home"),
  [BTN_PROBE_Z] = BUTTON_INFO(BTN_PROBE_Z_GPIO, "Button-Probe-Z"),
  [BTN_PROBE_XYZ] = BUTTON_INFO(BTN_PROBE_XYZ_GPIO, "Button-Probe-XYZ"),
};

/* Multipliers for slow, medium fast speed selectors */
const int speed_multiplier[] = {1, 10, 25};

volatile uint8_t xy_speed = 0;
volatile uint8_t z_speed = 0;
repeating_timer_t poll_ain_timer;

volatile int32_t encoder_position = 0;
uint8_t last_encoder_state = 0;

static void handle_encoder(void)
{
  int8_t rotary_lookup[4][4] = { { 0, -1,  1,  2},
                                 { 1,  0,  2, -1},
                                 {-1,  2,  0,  1},
                                 { 2,  1, -1,  0}};

  uint8_t current_encoder_state = (gpio_get(ROT_A_GPIO) << 1) | gpio_get(ROT_B_GPIO);
  encoder_position += rotary_lookup[last_encoder_state][current_encoder_state];
  last_encoder_state = current_encoder_state;
}

static volatile btn_info_t *gpio_to_button_info(uint gpio)
{
  btn_t btn = NUM_BTNS;
  switch(gpio) {
  case JOY_LEFT_GPIO:
    btn = JOY_LEFT;
    break;
  case JOY_DOWN_GPIO:
    btn = JOY_DOWN;
    break;
  case JOY_RIGHT_GPIO:
    btn = JOY_RIGHT;
    break;
  case JOY_UP_GPIO:
    btn = JOY_UP;
    break;
  case BTN_XY_ZERO_GPIO:
    btn = BTN_XY_ZERO;
    break;
  case BTN_Z_ZERO_GPIO:
    btn = BTN_Z_ZERO;
    break;
  case BTN_HOME_GPIO:
    btn = BTN_HOME;
    break;
  case BTN_PROBE_Z_GPIO:
    btn = BTN_PROBE_Z;
    break;
  case BTN_PROBE_XYZ_GPIO:
    btn = BTN_PROBE_XYZ;
    break;
  }

  if (btn < ARRAY_SIZE(button_info))
    return &button_info[btn];

  printf("Error: Unhandled button for GPIO-%d\n", gpio);
  return NULL;
}

static int64_t debounce_handler(alarm_id_t id, void *user_data)
{
  volatile btn_info_t *button = (btn_info_t *)user_data;

  button->debouncing = false;
  /* If it's still low, then increment active count and start polling */
  if (!gpio_get(button->gpio)) {
    button->repeat_time = delayed_by_ms(get_absolute_time(), BTN_AUTOREPEAT_MS);
    button->count++;
    button->poll = true;
  }
  return 0;
}

static void gpio_isr(uint gpio, uint32_t events)
{
  volatile btn_info_t *button = NULL;

  switch (gpio) {
  case JOY_LEFT_GPIO:
  case JOY_UP_GPIO:
  case JOY_RIGHT_GPIO:
  case JOY_DOWN_GPIO:
  case BTN_XY_ZERO_GPIO:
  case BTN_Z_ZERO_GPIO:
  case BTN_HOME_GPIO:
  case BTN_PROBE_Z_GPIO:
  case BTN_PROBE_XYZ_GPIO:
    button = gpio_to_button_info(gpio);
    if ((events & GPIO_IRQ_EDGE_FALL) && (!button->debouncing)) {
      button->debouncing = true;
      alarm_id_t id = add_alarm_at(
                        delayed_by_ms(get_absolute_time(), BTN_DEBOUNCE_MS),
                        debounce_handler, (void *)button, true);
    }
    break;

  case ROT_A_GPIO:
  case ROT_B_GPIO:
    handle_encoder();
    break;
  }
}

static bool check_ain(int chan)
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
    printf("XY Speed updated to %d (multiply %d)\n", xy_speed, speed_multiplier[xy_speed]);
    return true;
  }

  if ((chan == AIN_Z_SPEED_CHAN) && (scaled != z_speed)) {
    z_speed = scaled;
    printf("Z Speed updated to %d (multiply %d)\n", z_speed, speed_multiplier[z_speed]);
    return true;
  }

  return false;
}

bool periodic_poll_ain(repeating_timer_t *rt) {
  check_ain(AIN_XY_SPEED_CHAN);
  check_ain(AIN_Z_SPEED_CHAN);
  return true;
}

/*
 * Public Functions
 */
void set_led(bool enable)
{
#if defined(PICO_DEFAULT_LED_PIN)
  gpio_put(PICO_DEFAULT_LED_PIN, enable);
#elif defined(CYW43_WL_GPIO_LED_PIN)
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, enable);
#else
  #error Unkown LED configuration
#endif
}

void grbl_uart_init(void)
{
  // Initialize UART for GRBL communication
  uint actual_baud = uart_init(GRBL_UART_ID, GRBL_UART_BAUD);
  if (actual_baud == 0) {
    printf("Error: Failed to initialize UART1 at %d baud\n", GRBL_UART_BAUD);
    return;
  }
  printf("GRBL UART initialized at %u baud (requested %d)\n", actual_baud, GRBL_UART_BAUD);

  gpio_set_function(UART_TX_GRBL_RX, GPIO_FUNC_UART);
  gpio_set_function(UART_RX_GRBL_TX, GPIO_FUNC_UART);
  uart_set_hw_flow(GRBL_UART_ID, false, false); // No HW flow control
  uart_set_format(GRBL_UART_ID, 8, 1, UART_PARITY_NONE); // 8-N-1
}

void io_init(void)
{

#if defined(PICO_DEFAULT_LED_PIN)
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#elif defined(CYW43_WL_GPIO_LED_PIN)
    cyw43_arch_init();
#endif

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
  irq_set_enabled(IO_IRQ_BANK0, true);

  adc_init();
  adc_gpio_init(AIN_XY_SPEED_PIN);
  adc_gpio_init(AIN_Z_SPEED_PIN);
 
  add_repeating_timer_ms(AIN_POLL_PERIOD_MS, periodic_poll_ain, NULL, &poll_ain_timer);
}

void handle_inputs(void)
{
  static int32_t last_encoder_position = 0;
  int16_t delta;

  if (encoder_position != last_encoder_position) {
    delta = encoder_position - last_encoder_position;
    last_encoder_position = encoder_position;
    // 1 click is 4 encoder positions so divide by 4
    grbl_quick_command(MOVE_Z, (delta * speed_multiplier[z_speed]) / 4 );
  }

  for (int i = 0; i < NUM_BTNS; i++) {
    volatile btn_info_t *button = &button_info[i];
    if (button->count) {
      delta = button->count;
      printf(" Button: %s (count=%d)\n", button->name, delta);
      grbl_cmd cmd = {INVALID, 0};
      switch(i) {
      case JOY_LEFT:
        grbl_quick_command(MOVE_X, delta * -speed_multiplier[xy_speed]);
        button->count -= delta;
        break;
      case JOY_UP:
        grbl_quick_command(MOVE_Y, delta * speed_multiplier[xy_speed]);
        button->count -= delta;
        break;
      case JOY_RIGHT:
        grbl_quick_command(MOVE_X, delta * speed_multiplier[xy_speed]);
        button->count -= delta;
        break;
      case JOY_DOWN:
        grbl_quick_command(MOVE_Y, delta * -speed_multiplier[xy_speed]);
        button->count -= delta;
        break;
      case BTN_XY_ZERO_GPIO:
        grbl_quick_command(ZERO_XY, 0);
        button->count = 0;
        break;
      case BTN_Z_ZERO_GPIO:
        grbl_quick_command(ZERO_Z, 0);
        button->count = 0;
        break;
      case BTN_HOME_GPIO:
        grbl_quick_command(HOME, 0);
        button->count = 0;
        break;
      case BTN_PROBE_Z_GPIO:
        grbl_quick_command(PROBE_Z, 0);
        button->count = 0;
        break;
      case BTN_PROBE_XYZ_GPIO:
        grbl_quick_command(PROBE_XY, 0);
        button->count = 0;
        break;
      }
    }
  }

  /* poll buttons that are being held active */
  for (int i = 0; i < NUM_BTNS; i++) {
    volatile btn_info_t *button = &button_info[i];

    if (button->poll) {
      if (!gpio_get(button->gpio)) {
        if (get_absolute_time() > button->repeat_time) {
          button->repeat_time = delayed_by_ms(get_absolute_time(), BTN_AUTOREPEAT_MS);
          button->count++;
        }
      } else {
        /* Stop polling once it goes inactive */
        button->poll = false;
      }
    }
  }
}
