
#include "HardwareProfile.h"

#include <plib.h>
#include <stdint.h>
#include "uart.h"
#include "AD7770.h"

#define OVEN_PWM_PERIOD 199
#define OVEN_MAX_DUTY ((int)(0.30*OVEN_PWM_PERIOD))

#define FIXED_POINT 16
#define I_SHIFT 4
   
int32_t gain_p = 10;
int32_t gain_i = -(2);
int32_t gain_d = 0;

int32_t target = 0;

uint8_t adc_index = 6;

int32_t error[3] = {0,0,0}; // 0 is the most recent error
int64_t last_duty = 0; 
int64_t integrator = 0;

uint8_t fb_enabled = 0;


void fb_config(int32_t new_gain_p, int32_t new_gain_i, int32_t new_gain_d ) {
    fb_stop();
    
    gain_p = new_gain_p;
    gain_i = new_gain_i;
    gain_d = new_gain_d;

    error[0] = 0;
    error[1] = 0;
    error[2] = 0;
    integrator = 0;

}

void fb_start() {
    if(fb_enabled) 
        return;
    
    error[0] = 0;
    error[1] = 0;
    error[2] = 0;
    integrator = 0; 
    
    fb_enabled = 1;
}

void fb_stop() {
    fb_enabled = 0;
    pwm_set_duty(0);
}

void fb_set_setpoint(int32_t new_setpoint) {
    
    target = new_setpoint;
}

void fb_update() {
    
    if(fb_enabled == 0)
        return;
    
    int32_t new_duty = last_duty;
    
    error[2] = error[1];
    error[1] = error[0];
    error[0] = target - last_samples_signed[adc_index];
    
    // If the output was saturated last time, dont add the integrator
    if( last_duty >= (OVEN_MAX_DUTY<<FIXED_POINT)) {
        if(gain_i*error[0] < 0) {
            // If the sign of the error is negative, then we can allow integration
            integrator += gain_i*error[0] >> I_SHIFT;
        }
    } else if( last_duty <= 0 ) {
        if(gain_i*error[0] > 0) {
            // If the sign of the error is positive, then we can allow integration
            integrator += gain_i*error[0] >> I_SHIFT;
        }
    } else
        integrator += gain_i*error[0] >> I_SHIFT;

    
    
    new_duty = gain_p*error[0] + integrator;
    
    
    if(new_duty < 0)
        new_duty = 0;
    if(new_duty > (OVEN_MAX_DUTY<<FIXED_POINT))
        new_duty = OVEN_MAX_DUTY<<FIXED_POINT;
            
    last_duty = new_duty;
    
    uint8_t new_duty_clipped = 0;
    
    if(new_duty > 0) 
        new_duty_clipped = new_duty >> FIXED_POINT;
    
    pwm_set_duty(new_duty_clipped);
}

void fb_read_status() {
    
    char buffer[100];

    sprintf(&buffer[0],"SP: %li\nM: %li\nEr: %li\nD: %lli\nI: %lli\n", target,last_samples_signed[adc_index], error[0], last_duty, integrator);
    uart_write(buffer,strlen(buffer));
    
}