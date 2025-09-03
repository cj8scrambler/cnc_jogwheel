#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/irq.h"

#include "tusb.h"

#include "io.h"

//extern volatile uint8_t joy_poll;  // from io.cpp

int main() {
int i=0;
  stdio_init_all();

  jogwheel_io_init();
  set_led(1);

  // Keep I/O on core0 and GRBL work on core1
  //multicore_launch_core1(grbl_handler);

  while (true) {
    // TODO: Move these to a timer
    check_ain(AIN_XY_SPEED_CHAN);
    check_ain(AIN_Z_SPEED_CHAN);

    handle_inputs();

    set_led(i++%2);
    sleep_ms(500);
  }

  return 0;
}
