
#ifndef _UART_H    
#define _UART_H


void uart_config();
void uart_write( char* buffer, int len);
int uart_read(char* buffer, int max_len);

#endif
