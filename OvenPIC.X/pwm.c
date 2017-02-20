
#include "HardwareProfile.h"

#include <plib.h>
#include <stdint.h>
#include "uart.h"
#include "pwm.h"



uint16_t pwm_0_duty = 0;


void pwm_set_duty(uint16_t duty) {
    
    if(duty > OVEN_MAX_DUTY)
        duty = OVEN_MAX_DUTY;
    
    pwm_0_duty = duty;
    OC4RS = duty;
}


// Use Timer 3 and OC4 to generate oven pwm on RB2 (pin6)
void pwm_config() {
    
    //TRISBbits.TRISB2 = 1;
    
    //PR3 = (SYSCLK/OVEN_PWM_CLOCK_FREQ) - 1;
    PR3 = OVEN_PWM_PERIOD;
    T3CONbits.ON = 1;
    
    RPB2R = 0b0101; // OC4 on RB2 (pin 6)
    OC4RS = 0;

    OC4CONbits.OCTSEL = 1; // Select timer 3
    OC4CONbits.OCM = 0b110; // PWM with fault disabled
    OC4CONbits.ON = 1;
}
