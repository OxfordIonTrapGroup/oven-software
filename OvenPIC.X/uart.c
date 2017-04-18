
#include "HardwareProfile.h"

#include <stdint.h>

#define UART_TX_BUFFER_LEN 256
char uart_tx_buffer[UART_TX_BUFFER_LEN];
int uart_tx_buffer_start = 0;
int uart_tx_buffer_end = 0;

#define UART_RX_BUFFER_LEN 256
char uart_rx_buffer[UART_RX_BUFFER_LEN];
int uart_rx_buffer_start = 0;
int uart_rx_buffer_end = 0;

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
        uart_rx_buffer_end++; 
        if(uart_rx_buffer_end >= UART_RX_BUFFER_LEN)
            uart_rx_buffer_end = 0;    
        
        IFS3bits.U1RXIF = 0;
    }
}


// UART5-TX on BB -> U1RX (RB9) (p22)
// UART5-RX on BB -> U1TX (RD11) (p45)
void uart_config() {
    
    U1RXR = 0b0101; // RB9 = U1RX
    RPD11R = 0b0001; // RD11 = U1TX
    
    U1MODE = 0;               // disable autobaud, TX and RX enabled only, 8N1, idle=HIGH
    U1MODEbits.BRGH = 1; // 4x divisor
    // PBCLK2 is 0.5*SYS_CLK = 90 MHz
    // for 0.9 MHz, brg = 24
    U1BRG = 24; // 0.9 MHz
    
    // Set up transmission
    U1STAbits.UTXEN = 1; // Enable TX
    // Don't enable the transmitter until first receiving data
    U1STAbits.UTXISEL = 0b00; // Enable TX interrupt when buffer has space
    //U1STAbits.UTXINV = 1; // Set TX idle state to off
    IFS3bits.U1TXIF = 0;
    IEC3bits.U1TXIE = 1; // Enable TX interrupt
    
    // Set up reception
    U1STAbits.URXEN = 1; // Enable RX
    U1STAbits.URXISEL = 0b00; // Interrupt happens when buffer is not empty
    IFS3bits.U1RXIF = 0;
    IEC3bits.U1RXIE = 1; // Enable RX interrupt
    
    IPC28bits.U1RXIP = 2; // Set interrupt priority level
    IPC28bits.U1TXIP = 2; // Set interrupt priority level    
    U1MODEbits.ON = 1;     // enable UART1
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
    if(IEC1bits.U1TXIE == 0) {
        IEC1bits.U1TXIE = 1;
    }
}

void uart_write_blocking(uint8_t* buffer, uint32_t len ) {
    
    uint32_t i=0;
    
    while(i < len) {
        while( !U1STAbits.TRMT ); // Wait for tx empty
        
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

