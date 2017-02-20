
#include "HardwareProfile.h"

#include <plib.h>
#include <stdint.h>
#include <string.h>
#include "AD7770.h"
#include "uart.h"
#include "feedback.h"

#pragma config FNOSC = PRIPLL // Primary oscillator
#pragma config POSCMOD = XT // 'XT' mode for xtals, ours is 8MHz
#pragma config FPLLIDIV = DIV_2 // Divide input clock by 2
#pragma config FPLLMUL = MUL_20 // Multiply input clock by 20
#pragma config FPLLODIV = DIV_2 // Divide by 2, giving 40 MHz sys clock
#pragma config FPBDIV = DIV_1 // Periferal clock = sys clock
#pragma config FSOSCEN = OFF // Secondary oscillator diabled, for dig IO on SOCS pins

#pragma config JTAGEN = OFF // Disable jtag  (needed for pin 24 (RED LED))
#pragma config ICESEL = ICS_PGx1 // ICE comm via pin-set 1 (needed for pin 24)



void ins_read_next();

void main() {
    
    ANSELA = 0;
    ANSELB = 0;
    SYSTEMConfig( SYSCLK, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);
    
    // Wait until the U1RX pin receives a signal
    TRISBbits.TRISB2 = 0;
    LATBbits.LATB2 = 0;
    
    while(PORTAbits.RA4 == 0);
    TRISBbits.TRISB2 = 1;

    TRISBbits.TRISB14 = 0;

    uart_config();

    adc_config();
    pwm_config();
    fb_config(0,0,0);
   
    INTEnableSystemMultiVectoredInt();

    while(1) {
        ins_read_next();
    }
}



