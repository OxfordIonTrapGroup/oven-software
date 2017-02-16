
#include "HardwareProfile.h"

#include <plib.h>
#include <stdint.h>
#include "uart.h"
#include "AD7770.h"
#include "commands.h"
#include "feedback.h"

#define OVEN_PWM_PERIOD 199
#define OVEN_MAX_DUTY ((int)(0.30*OVEN_PWM_PERIOD))

#define FIXED_POINT 16
#define I_SHIFT 4
   
int32_t FB_gain_p = 10;
int32_t FB_gain_i = -(2);
int32_t FB_gain_d = 0;

int32_t FB_target = 0;
uint8_t FB_adc_index = 6;
int32_t FB_last_error = 0; 
int64_t FB_last_duty = 0; 
int64_t FB_integrator = 0;
uint8_t FB_enabled = 0;


void fb_config(int32_t new_gain_p, int32_t new_gain_i, int32_t new_gain_d ) {
    fb_stop();
    
    FB_gain_p = new_gain_p;
    FB_gain_i = new_gain_i;
    FB_gain_d = new_gain_d;

    FB_last_error = 0;
    FB_integrator = 0;

}

void fb_start() {
    if(FB_enabled) 
        return;
    
    FB_last_error = 0;
    FB_integrator = 0; 
    
    FB_enabled = 1;
}

void fb_stop() {
    FB_enabled = 0;
    pwm_set_duty(0);
}

void fb_set_setpoint(int32_t new_setpoint) {
    
    FB_target = new_setpoint;
}

void fb_update() {
    
    if(FB_enabled == 0)
        return;
    
    int32_t new_duty = FB_last_duty;
    
    FB_last_error = FB_target - last_samples_signed[FB_adc_index];
    
    // If the output was saturated last time, dont add the integrator
    if( FB_last_duty >= (OVEN_MAX_DUTY<<FIXED_POINT)) {
        if(FB_gain_i*FB_last_error < 0) {
            // If the sign of the error is negative, then we can allow integration
            FB_integrator += FB_gain_i*FB_last_error >> I_SHIFT;
        }
    } else if( FB_last_duty <= 0 ) {
        if(FB_gain_i*FB_last_error > 0) {
            // If the sign of the error is positive, then we can allow integration
            FB_integrator += FB_gain_i*FB_last_error >> I_SHIFT;
        }
    } else
        FB_integrator += FB_gain_i*FB_last_error >> I_SHIFT;

    
    
    new_duty = FB_gain_p*FB_last_error + FB_integrator;
    
    
    if(new_duty < 0)
        new_duty = 0;
    if(new_duty > (OVEN_MAX_DUTY<<FIXED_POINT))
        new_duty = OVEN_MAX_DUTY<<FIXED_POINT;
            
    FB_last_duty = new_duty;
    
    uint8_t new_duty_clipped = 0;
    
    if(new_duty > 0) 
        new_duty_clipped = new_duty >> FIXED_POINT;
    
    pwm_set_duty(new_duty_clipped);
}

void fb_read_status() {
    
    ins_send_text_message(CMD_FEEDBACK_READ_STATUS, \
            "SP: %li\nM: %li\nEr: %li\nD: %lli\nI: %lli\n", \
            FB_target, last_samples_signed[FB_adc_index], FB_last_error, FB_last_duty, FB_integrator);
    
//    char buffer[100];
//    
//    
//
//    sprintf(&buffer[0],"SP: %li\nM: %li\nEr: %li\nD: %lli\nI: %lli\n", target,last_samples_signed[adc_index], last_error, last_duty, integrator);
//    uart_write(buffer,strlen(buffer));
    
}