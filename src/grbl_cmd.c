#include <stdio.h>

#include "grbl_cmd.h"

int grbl_cmd_move(float x_distance, float y_distance, float z_distance, uint16_t speed) {
  // TODO: Implement jogging the head in specified direction(s)
  // This should send g-code commands to move the head while it is not spinning
  printf("%s(%.1f, %.1f, %.1f, %d)\n", __func__, x_distance, y_distance, z_distance, speed);
  return 0;
}

int grbl_cmd_zero(bool zero_x, bool zero_y, bool zero_z) {
  // TODO: Implement setting the current position as the reference '0' position
  // This should send g-code commands to set work coordinate system zeros
  printf("%s(%d, %d, %d)\n", __func__, zero_x, zero_y, zero_z);
  return 0;
}

int grbl_cmd_probe(bool probe_x, bool probe_y, bool probe_z, float x_offset, float y_offset, float z_offset) {
  // TODO: Implement automatic touch probe operations
  // This should:
  // 1. Pre-position the head to ensure contact with touch probe
  // 2. Slowly move towards touch block until contact detected
  // 3. Back off and retry multiple times for consistency
  // 4. Repeat for each enabled axis (X, Y, Z)
  // 5. Apply x_offset, y_offset, and z_offset to compensate for probe block position
  printf("%s(%d, %d, %d, %.1f, %.1f, %.1f)\n", __func__, probe_x, probe_y, probe_z, x_offset, y_offset, z_offset);
  return 0;
}
