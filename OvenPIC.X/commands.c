
#include "HardwareProfile.h"

#include <stdint.h>
#include <string.h>
#include "uart.h"
#include "AD7770.h"
#include "feedback.h"
#include "interface.h"

#include "commands.h"

#define ERROR -1

// int _parse_args(ins_header_t* header, char* data, void* args) {
    
//     uint8_t length = 0;
    
//     switch(header->command) {
        
//         // PWM commands
//         case CMD_PWM_SET_DUTY:
//             length = sizeof(cmd_pwm_set_duty_args_t);
//             break;
            
//         // ADC commands
//         case CMD_ADC_STREAM:
//             length = sizeof(cmd_adc_stream_args_t);
//             break;
//         case CMD_ADC_DECIMATE:
//             length = sizeof(cmd_adc_decimate_args_t);
//             break;
            
//         // Feedback commands
//         case CMD_FEEDBACK_CONFIG:
//             length = sizeof(cmd_feedback_config_args_t);
//             break;    
            
//         case CMD_FEEDBACK_SETPOINT:
//             length = sizeof(cmd_feedback_setpoint_args_t);
//             break;
            
//         case CMD_FEEDBACK_SET_LIMITS:
//             length = sizeof(cmd_feedback_set_limits_args_t);
//             break;
            
//         default:
//             length = 0;
//             break;
//     }
    
//     if(header->len != length) {
//         ins_report_error("Bad args length in command %x", header->command);
//         return ERROR;
//     }
    
//     // Copy the data across into the args pointer
//     memcpy(args, (void*)data, header->len);
    
//     return 0;
// }


void cmd_echo(char* line, uint32_t length) {
    if(length > 0) {
        uart_printf(">echo %s\n", line);
    } else {
        uart_printf(">echo\n");
    }
}

// PWM commands

void cmd_pwm_set_duty(char* line, uint32_t length) {

    uint32_t channel;
    float new_duty;

    // Read in the values
    sscanf(line, "%i %f", &channel, &new_duty);

    if(channel > 1) {
        uart_printf("! Bad channel: %i\n", channel);
        return;
    }

    if(new_duty < 0 || new_duty > 1) {
        uart_printf("! Bad duty cycle: %f\n", new_duty);
        return;
    }

    pwm_set_duty(channel, new_duty);
    uart_printf(">%i %f\n", channel, new_duty);
}




// ADC commands

void cmd_adc_stream(char* line, uint32_t length) {

    uint8_t channels;

    // Read in the values
    sscanf(line, "%i", &channels);

    adc_streaming_start(channels);
    uart_printf(">%i\n", channels);
}

void cmd_adc_decimate(char* line, uint32_t length) {

    uint32_t decimation;

    // Read in the decimation
    sscanf(line, "%i", &decimation);

    adc_set_streaming_decimation(decimation);
    uart_printf(">%i\n", decimation);
}

void cmd_adc_read_last_conversion(char* line, uint32_t length) {
    
    uint32_t i;
    
    uart_printf(">");
    for(i=0;i<8;i++)
        uart_printf(" %f", last_samples_float[i]*2.5);

    // Also print the crc failure count
    uart_printf(" %i\n", adc_crc_failure_count);
}

/*
// PWM commands

void cmd_pwm_set_duty(ins_header_t* header, char* data) {
    cmd_pwm_set_duty_args_t args;
    
    // Parse the arguments
    if(_parse_args(header, data, &args) == ERROR)
        return;
    
    pwm_set_duty(args.new_duty);
}

// Feedback commands

void cmd_feedback_config(ins_header_t* header, char* data) {
    cmd_feedback_config_args_t args;
    
    // Parse the arguments
    if(_parse_args(header, data, &args) == ERROR)
        return;

    fb_config(args.gain_p, args.gain_i, args.gain_d);
}

void cmd_feedback_start(ins_header_t* header, char* data) {
    fb_start();
}

void cmd_feedback_stop(ins_header_t* header, char* data) {
    fb_stop();
}

void cmd_feedback_setpoint(ins_header_t* header, char* data) {
    cmd_feedback_setpoint_args_t args;
    
    // Parse the arguments
    if(_parse_args(header, data, &args) == ERROR)
        return;
  
    fb_set_setpoint(args.setpoint);
}

void cmd_feedback_read_status(ins_header_t* header, char* data) {
    cmd_feedback_read_status_reply_t reply;

    reply.setpoint = FB_target;
    reply.last_sample = last_samples_signed[FB_adc_index];
    reply.last_error = FB_last_error;
    reply.last_duty = FB_last_duty;
    reply.integrator = FB_integrator;
    
    ins_send_reply(header, &reply, sizeof(reply));
}


void cmd_feedback_set_limits(ins_header_t* header, char* data) {
    cmd_feedback_set_limits_args_t args;
    
    // Parse the arguments
    if(_parse_args(header, data, &args) == ERROR)
        return;
  
    fb_set_limits(args.limit_i, args.limit_t);
}

*/