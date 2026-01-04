#include <stdio.h>
#include "pico/stdlib.h"

#include "io.h"
#include "grbl.h"

int main() {
int i=0;
  stdio_init_all();

  io_init();
  set_led(1);

  grbl_init();

  while (true) {
    handle_inputs();

    set_led(i++%2);
    sleep_ms(500);
  }

  return 0;
}
