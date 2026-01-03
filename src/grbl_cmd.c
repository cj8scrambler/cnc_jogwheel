#include "grbl_cmd.h"

int grbl_cmd_move(float x_distance, float y_distance, float z_distance, float speed) {
  // TODO: Implement jogging the head in specified direction(s)
  // This should send g-code commands to move the head while it is not spinning
  return 0;
}

int grbl_cmd_zero(bool zero_x, bool zero_y, bool zero_z) {
  // TODO: Implement setting the current position as the reference '0' position
  // This should send g-code commands to set work coordinate system zeros
  return 0;
}

int grbl_cmd_probe(bool probe_x, bool probe_y, bool probe_z, float z_offset) {
  // TODO: Implement automatic touch probe operations
  // This should:
  // 1. Pre-position the head to ensure contact with touch probe
  // 2. Slowly move towards touch block until contact detected
  // 3. Back off and retry multiple times for consistency
  // 4. Repeat for each enabled axis (X, Y, Z)
  // 5. Apply z_offset to compensate for touch block thickness on Z axis
  return 0;
}
