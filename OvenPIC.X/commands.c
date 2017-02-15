
#include "HardwareProfile.h"

#include <plib.h>
#include <stdint.h>
#include <string.h>
#include "uart.h"
#include "AD7770.h"
#include "feedback.h"

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
        ins_report_error("bad args length");
        return ERROR;
    }
    
    // Copy the data across into the args pointer
    memcpy(args, (void*)data, header->len);
    
    return 0;
}


void cmd_echo(ins_header_t* header, uint8_t* data) {
    uart_write(data, header->len);
}

// ADC commands

void cmd_adc_read_last_conversion(ins_header_t* header, uint8_t* data) {
    float floatData[8];
    char buffer[120];
    adc_convert_samples(last_samples, floatData);

    sprintf(&buffer[0],"%f %f %f %f \n", floatData[4], floatData[5], floatData[6], floatData[7]);
    uart_write(buffer,strlen(buffer));
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

//    int32_t gains[3];
//    int i;
//    
//    for(i=0;i<3;i++) {
//    
//        gains[i] = data[4*i + 3];
//        gains[i] += data[4*i + 2] << 8;
//        gains[i] += data[4*i + 1] << 16;
//        gains[i] += (data[4*i] & 0x7F) << 24;
//        if(data[4*i]&0x80)
//            gains[i] = gains[i] - (1<<31);
//    }
    
    
    //fb_config(gains[0], gains[1], gains[2]);
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
    
//    int32_t new_setpoint;
//   
//    new_setpoint = data[3];
//    new_setpoint += data[2] << 8;
//    new_setpoint += data[1] << 16;
//    new_setpoint += (data[0] & 0x7F) << 24;
//    
//    if(data[0]&0x80)
//        new_setpoint = new_setpoint - (1<<31);
//    
//    fb_set_setpoint(new_setpoint);
    
    fb_set_setpoint(args.setpoint);
    
//    char buffer[100];
//
//    //sprintf(&buffer[0],"%i\n", new_setpoint);
//    sprintf(&buffer[0],"%i\n", new_setpoint);
//    uart_write(buffer,strlen(buffer));
}




