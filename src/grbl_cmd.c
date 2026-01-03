#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

#include "grbl_cmd.h"

// UART configuration for GRBL communication
// Using UART1 as UART0 is typically used for USB console on Pico
#define UART_ID uart1
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define UART_BAUD_RATE 115200  // Standard baud rate for GRBL
#define UART_TIMEOUT_MS 5000   // Timeout for waiting for GRBL response

/**
 * Initialize UART for GRBL communication
 * Should be called once during system initialization
 */
void grbl_uart_init(void) {
  // Initialize UART
  uart_init(UART_ID, UART_BAUD_RATE);
  
  // Set TX and RX pins
  gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
  gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
}

/**
 * Read response from GRBL controller and check for "ok"
 * @return 0 if "ok" received, -1 on timeout, -2 on error response
 */
static int grbl_wait_for_ok(void) {
  char response[64];
  int idx = 0;
  absolute_time_t timeout = make_timeout_time_ms(UART_TIMEOUT_MS);
  
  // Read characters until we get a complete line or timeout
  while (idx < (int)sizeof(response) - 1) {
    if (time_reached(timeout)) {
      printf("GRBL response timeout\n");
      return -1;
    }
    
    if (uart_is_readable(UART_ID)) {
      char c = uart_getc(UART_ID);
      
      // Check for end of line
      if (c == '\n' || c == '\r') {
        if (idx > 0) {
          response[idx] = '\0';
          
          // Check if response is "ok"
          if (strncmp(response, "ok", 2) == 0) {
            return 0;
          }
          
          // Check if response is an error
          if (strncmp(response, "error", 5) == 0) {
            printf("GRBL error response: %s\n", response);
            return -2;
          }
          
          // Got a response but not ok or error, keep reading
          idx = 0;
          continue;
        }
      } else {
        response[idx++] = c;
      }
    } else {
      // No data available, yield to other tasks
      sleep_us(100);
    }
  }
  
  printf("GRBL response buffer overflow\n");
  return -1;
}

int grbl_cmd_move(float x_distance, float y_distance, float z_distance, uint16_t speed) {
  // Build GRBL jog command: $J=G91 [X...] [Y...] [Z...] F...
  // G91 = incremental positioning mode (relative moves)
  // Only include axes with non-zero distances
  // F = feed rate in mm/min
  
  char command[128];
  int len = 0;
  int ret;
  
  // Start with jog command prefix and incremental mode
  ret = snprintf(command, sizeof(command), "$J=G91");
  if (ret < 0 || ret >= (int)sizeof(command)) return -1;
  len = ret;
  
  // Add X axis if distance is non-zero
  if (x_distance != 0.0f) {
    ret = snprintf(command + len, sizeof(command) - len, " X%.3f", x_distance);
    if (ret < 0 || ret >= (int)(sizeof(command) - len)) return -1;
    len += ret;
  }
  
  // Add Y axis if distance is non-zero
  if (y_distance != 0.0f) {
    ret = snprintf(command + len, sizeof(command) - len, " Y%.3f", y_distance);
    if (ret < 0 || ret >= (int)(sizeof(command) - len)) return -1;
    len += ret;
  }
  
  // Add Z axis if distance is non-zero
  if (z_distance != 0.0f) {
    ret = snprintf(command + len, sizeof(command) - len, " Z%.3f", z_distance);
    if (ret < 0 || ret >= (int)(sizeof(command) - len)) return -1;
    len += ret;
  }
  
  // Add feed rate (speed)
  ret = snprintf(command + len, sizeof(command) - len, " F%u", speed);
  if (ret < 0 || ret >= (int)(sizeof(command) - len)) return -1;
  len += ret;
  
  // Add newline terminator
  ret = snprintf(command + len, sizeof(command) - len, "\n");
  if (ret < 0 || ret >= (int)(sizeof(command) - len)) return -1;
  len += ret;
  
  // Send command via UART to GRBL controller
  uart_puts(UART_ID, command);
  
  printf("%s(%.1f, %.1f, %.1f, %u) -> %s", __func__, x_distance, y_distance, z_distance, speed, command);
  
  // Wait for and parse response from GRBL
  ret = grbl_wait_for_ok();
  if (ret != 0) {
    printf("GRBL command failed with code: %d\n", ret);
    return ret;
  }
  
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
