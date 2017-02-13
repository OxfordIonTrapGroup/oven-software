

#include <plib.h>
#include <stdint.h>
#include <string.h>
#include "HardwareProfile.h"
#include "AD7770.h"

#pragma config FNOSC = PRIPLL // Primary oscillator
#pragma config POSCMOD = XT // 'XT' mode for xtals, ours is 8MHz
#pragma config FPLLIDIV = DIV_2 // Divide input clock by 2
#pragma config FPLLMUL = MUL_20 // Multiply input clock by 20
#pragma config FPLLODIV = DIV_2 // Divide by 2, giving 40 MHz sys clock
#pragma config FPBDIV = DIV_1 // Periferal clock = sys clock
#pragma config FSOSCEN = OFF // Secondary oscillator diabled, for dig IO on SOCS pins

#pragma config JTAGEN = OFF // Disable jtag  (needed for pin 24 (RED LED))
#pragma config ICESEL = ICS_PGx1 // ICE comm via pin-set 1 (needed for pin 24)



void uart_config();
void uart_write( char* buffer, int len);
void timer_config();
void ins_read_next();

void main() {
    
    ANSELA = 0;
    ANSELB = 0;
    SYSTEMConfig( SYSCLK, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);
    
    // Wait until the U1RX pin receives a signal
    while(PORTAbits.RA4 == 0);
    //while(1);
    //TRISBbits.TRISB9 = 0;
    TRISBbits.TRISB14 = 0;
    
    //LED_GREEN = 1;
    //LED_RED = 1;
    uart_config();

    adc_config();
 //   timer_config();
//    asm("ei");
   
    INTEnableSystemMultiVectoredInt();

    int x =0;
    while(1) {
       // LED_RED = !LED_RED;
        //LED_GREEN = !LED_GREEN;
        
        int i = 4000000;
        while(i > 0) i--;
        
        //SPI2BUF = 0x1234;
//        char buffer[120];
//        uint32_t data[8];
//        float floatData[8];
//        adc_read_samples(&data);
//        uint8_t fails = adc_check_samples(&data);
//        adc_convert_samples(&data, &floatData);
//        
//        //data[0] = adc_read(x);
//        sprintf(&buffer[0],"%x %f %f %f %f \n", fails, floatData[4], floatData[5], floatData[6], floatData[7]);
//        uart_write(buffer,strlen(buffer));
//        
//        x = uart_read(buffer,120);
//        uart_write(buffer,x);
        ins_read_next();
        
        //x++;
        //x %= 0x64;
    }
}






int cc = 0;
void __ISR(_TIMER_3_VECTOR, IPL1AUTO) t3_interrupt() {
    IFS0bits.T3IF = 0; // Clear T3 interrupt
    LED_GREEN = !LED_GREEN;
    uart_write("!",1);
}


void timer_config() {
    PR3 = (156250/10) - 1;
    IEC0bits.T3IE = 1; // Enable timer 3 interrupt
    IFS0bits.T3IF = 0;
    IPC3bits.T3IP = 1;

    T3CONbits.TCKPS = 0b111; // 1:256 prescale (40MHz => 156.25 kHz)
    T3CONbits.ON = 1;
}


#define OVEN_PWM_CLOCK_FREQ 1000000

// Use Timer 3 and OC2 to generate oven pwm on RB5
void oven_0_config() {
    
    PR3 = (SYSCLK/OVEN_PWM_CLOCK_FREQ) - 1;
 
    OC2RS = (PR3 + 1)*0.05;
    T3CONbits.ON = 1;
    
    
    OC2CONbits.OCTSEL = 1;
    OC2CONbits.OCM = 6;
    OC2CONbits.ON = 1;
    
    RPB5R = 0b101; // OC2 on RB5 (pin 14)
}