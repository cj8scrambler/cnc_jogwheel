#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"

#include "grbl.h"

int command_result;
static queue_t message_queue;

static const char *action_names[] = {
  [MOVE_X]   = "MOVE_X",
  [MOVE_Y]   = "MOVE_Y",
  [MOVE_Z]   = "MOVE_Z",
  [ZERO_Z]   = "ZERO_Z",
  [ZERO_XY]  = "ZERO_XY",
  [PROBE_Z]  = "PROBE_Z",
  [PROBE_XY] = "PROBE_XY",
  [INVALID]  = "INVALID",
};

static void grbl_handler();

int grbl_init() {
  // TODO: error checking
  queue_init(&message_queue, sizeof(grbl_cmd), 16);
  multicore_launch_core1(grbl_handler);
  return 0;
}

void grbl_shutdown() {
  queue_free(&message_queue);
}

const char *action_name(grbl_action action) {
  return action_names[action];
}

int grbl_quick_command(grbl_action action, int16_t arg1) {
  grbl_cmd command = {
    .action = action,
    .arg1 = arg1
  };
  return (queue_try_add(&message_queue, &command));
}

int grbl_queue_command(grbl_cmd *command) {
  queue_add_blocking(&message_queue, command);
  return 0;
}

static void grbl_handler() {
  grbl_cmd command;

  while (true) {
    queue_remove_blocking(&message_queue, &command);
    printf("Running command %s\n", action_name(command.action));
    // TODO: actually do something
    sleep_ms(100);
    command_result = 0;
    printf("Command complete %s\n", action_name(command.action));
  }
}

