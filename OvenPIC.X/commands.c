
#include "HardwareProfile.h"

#include <stdint.h>
#include <string.h>
#include "uart.h"
#include "AD7770.h"
#include "feedback_controller.h"
#include "interface.h"

#include "commands.h"

#define ERROR -1

void cmd_echo(char* line, uint32_t length) {
    if(length > 0) {
        uart_printf(">echo %s\n", line);
    } else {
        uart_printf(">echo\n");
    }
}

void cmd_version(char* line, uint32_t length) {
    uart_printf(">version %s\n", VERSION_STRING);
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


// Feedback commands
extern controller_t* current_controller;
extern controller_t* temperature_controller;


void cmd_feedback_config(char* line, uint32_t length) {

    float p, i, d;
    char name[FBC_NAME_LEN];

    sscanf(line, "%s %f %f %f", &name, &p, &i, &d);

    // Find the feedback controller with the given name
    controller_t* c = fbc_get_by_name(name);

    if(c == NULL) {
        // If get_by_name has returned null, then the name was not found
        uart_printf("! No controller with name \"%s\"\n", name);
        return;
    }

    c->p_gain = p;
    c->i_gain = i;
    c->d_gain = d;

    uart_printf(">%f %f %f\n", p, i, d);
}

void cmd_feedback_start(char* line, uint32_t length) {

    char name[FBC_NAME_LEN];
    sscanf(line, "%s", &name);

    // Find the feedback controller with the given name
    controller_t* c = fbc_get_by_name(name);

    if(c == NULL) {
        // If get_by_name has returned null, then the name was not found
        uart_printf("! No controller with name \"%s\"\n", name);
        return;
    }

    c->enabled = 1;
    uart_printf(">%s enabled\n", name);
}

void cmd_feedback_stop(char* line, uint32_t length) {

    char name[FBC_NAME_LEN];
    sscanf(line, "%s", &name);

    // Find the feedback controller with the given name
    controller_t* c = fbc_get_by_name(name);

    if(c == NULL) {
        // If get_by_name has returned null, then the name was not found
        uart_printf("! No controller with name \"%s\"\n", name);
        return;
    }

    c->enabled = 0;

    uart_printf(">%s disabled\n", name);
}

void cmd_feedback_setpoint(char* line, uint32_t length) {
    float setpoint;
    char name[FBC_NAME_LEN];
    sscanf(line, "%s %f", &name, &setpoint);

    // Find the feedback controller with the given name
    controller_t* c = fbc_get_by_name(name);

    if(c == NULL) {
        // If get_by_name has returned null, then the name was not found
        uart_printf("! No controller with name \"%s\"\n", name);
        return;
    }

    c->setpoint = setpoint;

    uart_printf(">%s %f\n", name, setpoint);
}

void cmd_feedback_read_status(char* line, uint32_t length) {

    char name[FBC_NAME_LEN];
    sscanf(line, "%s", &name);

    // Find the feedback controller with the given name
    controller_t* c = fbc_get_by_name(name);

    if(c == NULL) {
        // If get_by_name has returned null, then the name was not found
        uart_printf("! No controller with name \"%s\"\n", name);
        return;
    }

    uart_printf(">%s %f %f %f %f %f %i\n", 
        name,
        c->setpoint,
        c->value,
        c->error,
        c->integrator,
        c->cv,
        c->enabled);
}

/*
void cmd_feedback_set_limits(ins_header_t* header, char* data) {
    cmd_feedback_set_limits_args_t args;
    
    // Parse the arguments
    if(_parse_args(header, data, &args) == ERROR)
        return;
  
    fb_set_limits(args.limit_i, args.limit_t);
}

*/

void cmd_settings_load(char* line, uint32_t length) {

    settings_read();
    uart_printf(">loaded\n");
}

void cmd_settings_save(char* line, uint32_t length) {

    settings_write();
    uart_printf(">saved\n");
}

void cmd_settings_print(char* line, uint32_t length) {

    settings_printout();
}