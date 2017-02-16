
#include "HardwareProfile.h"

#include <plib.h>
#include <stdint.h>
#include <string.h>
#include "uart.h"
#include "AD7770.h"
#include "feedback.h"
#include "interface.h"

#include "commands.h"

#define ERROR -1

int _parse_args(ins_header_t* header, uint8_t* data, void* args) {
    
    uint8_t length = 0;
    
    switch(header->command) {
        
        // PWM commands
        case CMD_PWM_SET_DUTY:
            length = sizeof(cmd_pwm_set_duty_args_t);
            break;
            
        // ADC commands
        case CMD_ADC_STREAM:
            length = sizeof(cmd_adc_stream_args_t);
            break;
        case CMD_ADC_DECIMATE:
            length = sizeof(cmd_adc_decimate_args_t);
            break;
            
        // Feedback commands
        case CMD_FEEDBACK_CONFIG:
            length = sizeof(cmd_feedback_config_args_t);
            break;    
            
        case CMD_FEEDBACK_SETPOINT:
            length = sizeof(cmd_feedback_setpoint_args_t);
            break;
            
        default:
            length = 0;
            break;
    }
    
    if(header->len != length) {
        ins_report_error("Bad args length in command %x", header->command);
        return ERROR;
    }
    
    // Copy the data across into the args pointer
    memcpy(args, (void*)data, header->len);
    
    return 0;
}


void cmd_echo(ins_header_t* header, uint8_t* data) {
    
    ins_send_reply(header, data, header->len);
}

// ADC commands

void cmd_adc_read_last_conversion(ins_header_t* header, uint8_t* data) {
    
    cmd_adc_read_last_conversion_reply_t reply;
    int i;
    
    for(i=0;i<8;i++)
        reply.samples[i] = last_samples_signed[i];
    
    
    ins_send_reply(header, &reply, sizeof(reply));
}

void cmd_adc_stream(ins_header_t* header, uint8_t* data) {
    
    cmd_adc_stream_args_t args;
    
    // Parse the arguments
    if(_parse_args(header, data, &args) == ERROR)
        return;
    
    adc_streaming_start(args.channels);
}


void cmd_adc_decimate(ins_header_t* header, uint8_t* data) {
    cmd_adc_decimate_args_t args;
    
    // Parse the arguments
    if(_parse_args(header, data, &args) == ERROR)
        return;
        
    adc_set_streaming_decimation(args.decimation);
}


// PWM commands

void cmd_pwm_set_duty(ins_header_t* header, uint8_t* data) {
    cmd_pwm_set_duty_args_t args;
    
    // Parse the arguments
    if(_parse_args(header, data, &args) == ERROR)
        return;
    
    pwm_set_duty(args.new_duty);
}

// Feedback commands

void cmd_feedback_config(ins_header_t* header, uint8_t* data) {
    cmd_feedback_config_args_t args;
    
    // Parse the arguments
    if(_parse_args(header, data, &args) == ERROR)
        return;

    fb_config(args.gain_p, args.gain_i, args.gain_d);
}

void cmd_feedback_start(ins_header_t* header, uint8_t* data) {
    fb_start();
}

void cmd_feedback_stop(ins_header_t* header, uint8_t* data) {
    fb_stop();
}

void cmd_feedback_setpoint(ins_header_t* header, uint8_t* data) {
    cmd_feedback_setpoint_args_t args;
    
    // Parse the arguments
    if(_parse_args(header, data, &args) == ERROR)
        return;
  
    fb_set_setpoint(args.setpoint);
}

void cmd_feedback_read_status(ins_header_t* header, uint8_t* data) {
    cmd_feedback_read_status_reply_t reply;

    reply.setpoint = FB_target;
    reply.last_sample = last_samples_signed[FB_adc_index];
    reply.last_error = FB_last_error;
    reply.last_duty = FB_last_duty;
    reply.integrator = FB_integrator;
    
    ins_send_reply(header, &reply, sizeof(reply));
}



