
#ifndef _UART_H    
#define _UART_H


extern void uart_config();
extern void uart_write( char* buffer, int len);
extern int uart_read(char* buffer, int max_len);

#endif
