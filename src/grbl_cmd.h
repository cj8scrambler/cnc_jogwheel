#ifndef GRBL_CMD_H
#define GRBL_CMD_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Initialize UART for GRBL communication
 * Should be called once during system initialization before any grbl_cmd_* functions
 */
void grbl_uart_init(void);

/**
 * Move - Jog the head in specified direction(s) by a distance at a given speed.
 * This operation is for moving while the head is not spinning.
 * 
 * @param x_distance Distance to move in X direction (mm), use 0 to skip X movement
 * @param y_distance Distance to move in Y direction (mm), use 0 to skip Y movement
 * @param z_distance Distance to move in Z direction (mm), use 0 to skip Z movement
 * @param speed Speed at which to move the head (mm/min)
 * @return 0 on success, negative error code on failure
 */
int grbl_cmd_move(float x_distance, float y_distance, float z_distance, uint16_t speed);

/**
 * Zero - Set the current head position as the reference '0' position for a given axis.
 * Once the 0 position is set for all 3 directions, normal CNC operation can begin.
 * 
 * @param zero_x Set X axis zero if true
 * @param zero_y Set Y axis zero if true
 * @param zero_z Set Z axis zero if true
 * @return 0 on success, negative error code on failure
 */
int grbl_cmd_zero(bool zero_x, bool zero_y, bool zero_z);

/**
 * Probe - Perform automatic zero probe using a touch probe.
 * Slowly moves the head towards the touch block until contact is detected,
 * then backs off and repeats for consistency.
 * 
 * @param probe_x Perform X axis probe if true
 * @param probe_y Perform Y axis probe if true
 * @param probe_z Perform Z axis probe if true
 * @param x_offset Offset to compensate for probe block position in X direction (mm)
 * @param y_offset Offset to compensate for probe block position in Y direction (mm)
 * @param z_offset Offset to compensate for touch block thickness in Z direction (mm)
 * @return 0 on success, negative error code on failure
 */
int grbl_cmd_probe(bool probe_x, bool probe_y, bool probe_z, float x_offset, float y_offset, float z_offset);

#endif /* GRBL_CMD_H */
