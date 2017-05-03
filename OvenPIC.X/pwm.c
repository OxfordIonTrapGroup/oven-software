
#include "HardwareProfile.h"

#include <stdint.h>
#include "uart.h"
#include "pwm.h"


uint8_t pwm_decimation_counter = 0;
uint16_t pwm_0_duty = 0;

void __ISR(_TIMER_3_VECTOR, IPL1AUTO) t3_interrupt() {
    pwm_decimation_counter--;
    if(pwm_decimation_counter < (pwm_0_duty & ((1 << PWM_DECIMATION_BITS)-1)) )
        OC4RS = (pwm_0_duty >> PWM_DECIMATION_BITS) + 1;
    else
        OC4RS = (pwm_0_duty >> PWM_DECIMATION_BITS);
    
    if(pwm_decimation_counter == 0) {    
        pwm_decimation_counter = ((1 << PWM_DECIMATION_BITS));
    }
    IFS0bits.T3IF = 0; // Clear T3 interrupt
}


void pwm_set_duty(float duty) {
    
    if(duty > OVEN_MAX_DUTY)
        duty = OVEN_MAX_DUTY;
    
    pwm_0_duty = (uint16_t)(duty*OVEN_PWM_PERIOD);
    OC4RS = pwm_0_duty >> PWM_DECIMATION_BITS;
}


// Use Timer 3 and OC4 to generate oven pwm on RB2 (pin6)
void pwm_config() {
    
    //TRISBbits.TRISB2 = 1;
    
    //PR3 = (SYSCLK/OVEN_PWM_CLOCK_FREQ) - 1;
    PR3 = OVEN_PWM_PERIOD;
    IEC0bits.T3IE = 1; // Enable timer 3 interrupt
    IFS0bits.T3IF = 0;
    IPC3bits.T3IP = 1;    
    T3CONbits.ON = 1;
    
    RPB2R = 0b0101; // OC4 on RB2 (pin 6)
    OC4RS = 0;

    OC4CONbits.OCTSEL = 1; // Select timer 3
    OC4CONbits.OCM = 0b110; // PWM with fault disabled
    OC4CONbits.ON = 1;
}
