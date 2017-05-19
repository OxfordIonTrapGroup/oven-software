
#ifndef _UART_H    
#define _UART_H


extern void uart_config();
extern void uart_write(uint8_t* buffer, uint32_t len);
extern void uart_write_blocking(uint8_t* buffer, uint32_t len );
extern uint32_t uart_read(uint8_t* buffer, uint32_t max_len);
extern void uart_write_data(uint8_t* buffer, uint32_t len);

extern void uart_printf(uint8_t* format, ...);
extern void uart_printf_blocking(uint8_t* format, ...);

#endif
