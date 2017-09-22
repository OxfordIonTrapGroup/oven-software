
#include "hardware_profile.h"

#include <stdint.h>
#include <stdarg.h>
#include "timer.h"

/*
Controller has two UART channels to BB host:
-> UART1 is the 'command' channel. This is used for text based
    communication between host.
-> UART5 is the 'data' channel. This is used only for transmitting
    binary data from the PIC to the BB.

*/

#define UART_TX_BUFFER_LEN 1024
char uart_tx_buffer[UART_TX_BUFFER_LEN];
int uart_tx_buffer_start = 0;
int uart_tx_buffer_end = 0;

#define UART_RX_BUFFER_LEN 256
char uart_rx_buffer[UART_RX_BUFFER_LEN];
int uart_rx_buffer_start = 0;
int uart_rx_buffer_end = 0;

#define UART_DATA_TX_BUFFER_LEN 256*2
char uart_data_tx_buffer[UART_DATA_TX_BUFFER_LEN];
int uart_data_tx_buffer_start = 0;
int uart_data_tx_buffer_end = 0;

void __ISR(_UART1_TX_VECTOR, IPL2AUTO) uart_tx_interrupt() {
    // Process TX interrupt
    if(IFS3bits.U1TXIF) {
        // If there is more data to transmit, then send it
        if(uart_tx_buffer_start != uart_tx_buffer_end) {
            U1TXREG = uart_tx_buffer[uart_tx_buffer_start];
            uart_tx_buffer_start++;
            if(uart_tx_buffer_start >= UART_TX_BUFFER_LEN)
                uart_tx_buffer_start = 0;
        } else
            IEC3bits.U1TXIE = 0;
           
        IFS3bits.U1TXIF = 0;
    }
}

void __ISR(_UART1_RX_VECTOR, IPL2AUTO) uart_rx_interrupt() {
    // Process RX interrupt
    if(IFS3bits.U1RXIF) {
        // Copy the data into the RX buffer
        uart_rx_buffer[uart_rx_buffer_end] = U1RXREG;
        //uart_write_data(&uart_rx_buffer[uart_rx_buffer_end], 1);
        uart_rx_buffer_end++; 
        if(uart_rx_buffer_end >= UART_RX_BUFFER_LEN)
            uart_rx_buffer_end = 0;    
        
        IFS3bits.U1RXIF = 0;
    }
}

void __ISR(_UART5_TX_VECTOR, IPL2AUTO) uart_data_tx_interrupt() {
    // Process TX interrupt
    if(IFS5bits.U5TXIF) {
        // If there is more data to transmit, then send it
        if(uart_data_tx_buffer_start != uart_data_tx_buffer_end) {
            U5TXREG = uart_data_tx_buffer[uart_data_tx_buffer_start];
            uart_data_tx_buffer_start++;
            if(uart_data_tx_buffer_start >= UART_DATA_TX_BUFFER_LEN)
                uart_data_tx_buffer_start = 0;
        } else
            IEC5bits.U5TXIE = 0;
           
        IFS5bits.U5TXIF = 0;
    }
}

// Declare blocking print to placate compiler
void uart_printf_blocking(uint8_t* format, ...);


// UART1-TX on BB -> U1RX (RB9) (p22)
// UART1-RX on BB -> U1TX (RD11) (p45)
// UART4-TX on BB -> U5RX (RC14) (p48)
// UART4-RX on BB -> U5TX (RC13) (p47)
void uart_config() {
    
    // Configure UART1 - command channel
    ANSELBbits.ANSB9 = 0; // Disable analog input on RB9
    U1RXR = 0b0101; // RB9 = U1RX
    RPD11R = 0b0001; // RD11 = U1TX
    
    U1MODE = 0;
    U1MODEbits.BRGH = 1; // 4x divisor
    // PBCLK2 is 0.5*SYS_CLK = 100 MHz
    // for 0.9 MHz, brg = 27
    U1BRG = 27; // 0.9 MHz
    //// for 115.2 kHz, brg = 216 + 1
    //U1BRG = 216; // 115.2 kHz
    
    // Set up transmission
    U1STAbits.UTXEN = 1; // Enable TX
    U1STAbits.UTXISEL = 0b00; // Enable TX interrupt when buffer has space
    IFS3bits.U1TXIF = 0;
    IEC3bits.U1TXIE = 0; // TX interrupt is enabled in uart_write routine
    IPC28bits.U1TXIP = 2; // Set interrupt priority level

    // Set up reception
    U1STAbits.URXEN = 1; // Enable RX
    U1STAbits.URXISEL = 0b00; // Interrupt happens when buffer is not empty
    IFS3bits.U1RXIF = 0;
    IEC3bits.U1RXIE = 1; // Enable RX interrupt
    IPC28bits.U1RXIP = 2; // Set interrupt priority level

    U1MODEbits.ON = 1;     // enable UART1

    // Configure UART5 - data channel
    U5RXR = 0b0111; // RC14 = U5RX
    RPC13R = 0b0011; // RC13 = U5TX

    U5MODE = 0;
    U5MODEbits.BRGH = 1; // 4x divisor
    U5BRG = 27;

    // Set up transmission
    U5STAbits.UTXEN = 1; // Enable TX
    U5STAbits.UTXISEL = 0b00; // Enable TX interrupt when buffer has space
    IFS5bits.U5TXIF = 0;
    IEC5bits.U5TXIE = 0; // TX interrupt is enabled in uart_write routine
    IPC45bits.U5TXIP = 2; // Set interrupt priority level
    U5MODEbits.ON = 1;     // enable UART5

    // Initialise the positions to 0
    uart_tx_buffer_start = 0;
    uart_tx_buffer_end = 0;

    uart_rx_buffer_start = 0;
    uart_rx_buffer_end = 0;

    uart_data_tx_buffer_start = 0;
    uart_data_tx_buffer_end = 0;

    uart_printf_blocking("booted! %i %x\n", sys_time, RCON);
}

void uart_write(uint8_t* buffer, uint32_t len) {
    
    uint32_t i;
    // Copy the data into the send buffer
    for(i=0; i<len; i++) {
        uart_tx_buffer[uart_tx_buffer_end] = buffer[i];
        uart_tx_buffer_end++;
        if(uart_tx_buffer_end >= UART_TX_BUFFER_LEN)
            uart_tx_buffer_end = 0;
    }
    // If no transmission is currently happening, initiate it
    if(IEC3bits.U1TXIE == 0) {
        IEC3bits.U1TXIE = 1;
    }
}

void uart_write_blocking(uint8_t* buffer, uint32_t len ) {
    
    uint32_t i=0;
    
    while(i < len) {
        while(!U1STAbits.TRMT); // Wait for tx empty
        
        U1TXREG = buffer[i];
        i++;
    }
}

uint32_t uart_read(uint8_t* buffer, uint32_t max_len) {
    uint32_t len;
    for(len = 0; len < max_len; len++) {
        if(uart_rx_buffer_start == uart_rx_buffer_end)
            break;
        buffer[len] = uart_rx_buffer[uart_rx_buffer_start];
        uart_rx_buffer_start++;
        if(uart_rx_buffer_start >= UART_RX_BUFFER_LEN)
            uart_rx_buffer_start = 0;
    }
    return len;
}

void uart_write_data(uint8_t* buffer, uint32_t len) {
    
    uint32_t i;
    // Copy the data into the send buffer
    for(i=0; i<len; i++) {
        uart_data_tx_buffer[uart_data_tx_buffer_end] = buffer[i];
        uart_data_tx_buffer_end++;
        if(uart_data_tx_buffer_end >= UART_DATA_TX_BUFFER_LEN)
            uart_data_tx_buffer_end = 0;
    }
    // If no transmission is currently happening, initiate it
    if(IEC5bits.U5TXIE == 0) {
        IEC5bits.U5TXIE = 1;
    }
}

void uart_printf(uint8_t* format, ...) {

    va_list arg_list;
    uint8_t message[UART_TX_BUFFER_LEN];

    va_start(arg_list, format);             // Prep arguments list
    vsprintf(message, format, arg_list);    // "Print" to buffer
    va_end(arg_list);                       // End handling of arguments list

    uart_write(message, strlen(message));
}

void uart_printf_blocking(uint8_t* format, ...) {

    va_list arg_list;
    uint8_t message[UART_TX_BUFFER_LEN];

    va_start(arg_list, format);             // Prep arguments list
    vsprintf(message, format, arg_list);    // "Print" to buffer
    va_end(arg_list);                       // End handling of arguments list

    uart_write_blocking(message, strlen(message));
}
