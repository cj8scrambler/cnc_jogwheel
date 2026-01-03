#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"

#include "grbl.h"
#include "grbl_cmd.h"

static queue_t message_queue;

#define MOVE_SPEED         100
#define PROBE_X_OFFSET     10
#define PROBE_Y_OFFSET     10
#define PROBE_Z_OFFSET     4

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
  // Initialize UART for GRBL communication
  grbl_uart_init();
  
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
  int ret;

  while (true) {
    queue_remove_blocking(&message_queue, &command);
    switch(command.action) {
    case MOVE_X:
      ret = grbl_cmd_move(command.arg1, 0, 0, MOVE_SPEED);
      break;
    case MOVE_Y:
      ret = grbl_cmd_move(0, command.arg1, 0, MOVE_SPEED);
      break;
    case MOVE_Z:
      ret = grbl_cmd_move(0, 0, command.arg1, MOVE_SPEED);
      break;
    case ZERO_XY:
      ret = grbl_cmd_zero(true, true, false);
      break;
    case ZERO_Z:
      ret = grbl_cmd_zero(false, false, true);
      break;
    case PROBE_XY:
      ret = grbl_cmd_probe(true, true, false, PROBE_X_OFFSET, PROBE_Y_OFFSET, PROBE_Z_OFFSET);
      break;
    case PROBE_Z:
      ret = grbl_cmd_probe(false, false, true, PROBE_X_OFFSET, PROBE_Y_OFFSET, PROBE_Z_OFFSET);
      break;
    default:
      printf("Invalid command: %d(%d)\n", command.action, command.arg1);
      break;
    }
  }
}

