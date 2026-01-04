#define GRBL_UART_ID        uart1

void set_led(bool enable);
void io_init(void);
void grbl_uart_init(void);
void handle_inputs(void);
