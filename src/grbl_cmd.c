#include <stdio.h>
#include <string.h>
#include "hardware/uart.h"

#include "grbl_cmd.h"

// UART configuration for GRBL communication
// Using UART1 as UART0 is typically used for USB console on Pico
#define UART_ID uart1
#define UART_TX_PIN 4
#define UART_RX_PIN 5

int grbl_cmd_move(float x_distance, float y_distance, float z_distance, uint16_t speed) {
  // Build GRBL jog command: $J=G91 [X...] [Y...] [Z...] F...
  // G91 = incremental positioning mode (relative moves)
  // Only include axes with non-zero distances
  // F = feed rate in mm/min
  
  char command[128];
  int len = 0;
  
  // Start with jog command prefix and incremental mode
  len = snprintf(command, sizeof(command), "$J=G91");
  
  // Add X axis if distance is non-zero
  if (x_distance != 0.0f) {
    len += snprintf(command + len, sizeof(command) - len, " X%.3f", x_distance);
  }
  
  // Add Y axis if distance is non-zero
  if (y_distance != 0.0f) {
    len += snprintf(command + len, sizeof(command) - len, " Y%.3f", y_distance);
  }
  
  // Add Z axis if distance is non-zero
  if (z_distance != 0.0f) {
    len += snprintf(command + len, sizeof(command) - len, " Z%.3f", z_distance);
  }
  
  // Add feed rate (speed)
  len += snprintf(command + len, sizeof(command) - len, " F%d", speed);
  
  // Add newline terminator
  len += snprintf(command + len, sizeof(command) - len, "\n");
  
  // Send command via UART to GRBL controller
  uart_puts(UART_ID, command);
  
  printf("%s(%.1f, %.1f, %.1f, %d) -> %s", __func__, x_distance, y_distance, z_distance, speed, command);
  
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
