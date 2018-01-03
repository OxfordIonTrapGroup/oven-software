
#include "hardware_profile.h"

#include <stdint.h>
#include "uart.h"
#include "pwm.h"
#include "settings.h"


uint8_t pwm_decimation_counter = 0;
uint16_t pwm_duty[2] = {0,0};

#define PWM_0_REG OC5RS
#define PWM_1_REG OC4RS

void __ISR(_TIMER_3_VECTOR, IPL4AUTO) t3_interrupt() {
    pwm_decimation_counter--;

    // Service pwm channel 0
    if(pwm_decimation_counter < (pwm_duty[0] & ((1 << PWM_DECIMATION_BITS)-1)) )
        PWM_0_REG = (pwm_duty[0] >> PWM_DECIMATION_BITS) + 1;
    else
        PWM_0_REG = (pwm_duty[0] >> PWM_DECIMATION_BITS);

    // Service pwm channel 1
    if(pwm_decimation_counter < (pwm_duty[1] & ((1 << PWM_DECIMATION_BITS)-1)) )
        PWM_1_REG = (pwm_duty[1] >> PWM_DECIMATION_BITS) + 1;
    else
        PWM_1_REG = (pwm_duty[1] >> PWM_DECIMATION_BITS);


    if(pwm_decimation_counter == 0) {    
        pwm_decimation_counter = ((1 << PWM_DECIMATION_BITS));
    }

    IFS0bits.T3IF = 0; // Clear T3 interrupt
}


void pwm_set_duty(uint32_t channel, float duty) {

    if(channel > 1) {
        return;
    }

    if(duty > settings.safety_settings.duty_max[channel])
        duty = settings.safety_settings.duty_max[channel];
    else if(duty < 0) {
        duty = 0;
    }

    pwm_duty[channel] = (uint16_t)(duty*(OVEN_PWM_PERIOD << PWM_DECIMATION_BITS));

    if(channel == 0) {
        PWM_0_REG = pwm_duty[channel] >> PWM_DECIMATION_BITS;
    } else {
        PWM_1_REG = pwm_duty[channel] >> PWM_DECIMATION_BITS;
    }
}


// Use Timer 3 and OC5 to generate oven pwm on RG6 (PWM0) 
// Use Timer 3 and OC4 to generate oven pwm on RG7 (PWM1) 
void pwm_config() {

    ANSELGbits.ANSG6 = 0;
    RPG6R = 0b1011; // OC5 on RG6
    OC5RS = 0;

    ANSELGbits.ANSG7 = 0;
    RPG7R = 0b1011; // OC4 on RG7
    OC4RS = 0;
    
    //PR3 = (SYSCLK/OVEN_PWM_CLOCK_FREQ) - 1;
    PR3 = OVEN_PWM_PERIOD;
    IEC0bits.T3IE = 1; // Enable timer 3 interrupt
    IFS0bits.T3IF = 0;
    IPC3bits.T3IP = 4;    
    T3CONbits.ON = 1;
    
    OC5CONbits.OCTSEL = 1; // Select timer 3
    OC5CONbits.OCM = 0b110; // PWM with fault disabled
    OC5CONbits.ON = 1;
    
    OC4CONbits.OCTSEL = 1; // Select timer 3
    OC4CONbits.OCM = 0b110; // PWM with fault disabled
    OC4CONbits.ON = 1;
}

void pwm_enable(uint32_t channel) {
    if(channel == 0) {
        OC5CONbits.ON = 1;
    } else {
        OC4CONbits.ON = 1;
    }
}

void pwm_disable(uint32_t channel) {
    if(channel == 0) {
        OC5CONbits.ON = 0;
    } else {
        OC4CONbits.ON = 0;
    }
}
