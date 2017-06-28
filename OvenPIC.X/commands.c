
#include "hardware_profile.h"

#include <stdint.h>
#include <string.h>
#include "uart.h"
#include "AD7770.h"
#include "feedback_controller.h"
#include "interface.h"
#include "settings.h"
#include "calibration.h"

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
        uart_printf(" %g", last_samples_float[i]);

    // Also print the crc failure count
    uart_printf(" %i\n", adc_crc_failure_count);
}

void cmd_adc_read_last_calibrated_data(char* line, uint32_t length) {
    
    uint32_t i;
    
    uart_printf(">");
    for(i=0;i<2;i++)
        uart_printf("%i %g %g %g %g;", i,
            calibrated_oven[i].temperature,
            calibrated_oven[i].current,
            calibrated_oven[i].output_voltage,
            calibrated_oven[i].oven_voltage);

    // Also print the crc failure count
    uart_printf(" %i\n", adc_crc_failure_count);
}


void cmd_feedback_config(char* line, uint32_t length) {

    float p, i, d;
    uint32_t sample_decimation;
    char name[FBC_NAME_LEN];

    sscanf(line, "%s %f %f %f %d",\
        &name, &p, &i, &d, &sample_decimation);

    // Find the feedback controller with the given name
    controller_t* c = fbc_get_by_name(name);

    if(c == NULL) {
        // If get_by_name has returned null, then the name was not found
        uart_printf("! No controller with name \"%s\"\n", name);
        return;
    }

    c->s->p_gain = p;
    c->s->i_gain = i;
    c->s->d_gain = d;
    c->s->sample_decimation = sample_decimation;

    uart_printf(">%f %f %f %d\n",\
        p, i, d, sample_decimation);
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

    c->target_setpoint = setpoint;

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

    uart_printf(">%s %f %f %f %f %f %f %i\n", 
        name,
        c->setpoint,
        c->target_setpoint,
        c->value,
        c->error,
        c->integrator,
        c->cv,
        c->enabled);
}


void cmd_feedback_set_limits(char* line, uint32_t length) {
    float cv_limit_min;
    float cv_limit_max;
    float value_limit_max;
    float setpoint_slewrate;
    char name[FBC_NAME_LEN];
    sscanf(line, "%s %g %g %g %g", &name, &cv_limit_min, &cv_limit_max,\
        &value_limit_max, &setpoint_slewrate);

    // Find the feedback controller with the given name
    controller_t* c = fbc_get_by_name(name);

    if(c == NULL) {
        // If get_by_name has returned null, then the name was not found
        uart_printf("! No controller with name \"%s\"\n", name);
        return;
    }

    c->s->cv_limit_min = cv_limit_min;
    c->s->cv_limit_max = cv_limit_max;
    c->s->value_limit_max = value_limit_max;
    c->s->setpoint_slewrate = setpoint_slewrate;

    uart_printf(">%s %g %g %g %g\n", name, cv_limit_min, cv_limit_max,\
        value_limit_max, setpoint_slewrate);
}



void cmd_settings_load(char* line, uint32_t length) {

    settings_read();
    uart_printf(">loaded\n");
}

void cmd_settings_set_to_factory(char* line, uint32_t length) {

    settings_set_to_factory();
    uart_printf(">set to factory\n");
}


void cmd_settings_save(char* line, uint32_t length) {

    settings_write();
    uart_printf(">saved\n");
}

void cmd_settings_print(char* line, uint32_t length) {

    settings_printout();
}


void cmd_safety_status(char* line, uint32_t length) {

    uart_printf(">");
    safety_print_errors();
    uart_printf("\n");
}

void cmd_safety_read_channel(char* line, uint32_t length) {

    uint32_t channel;
    sscanf(line, "%i", &channel);

    uart_printf(">");
    safety_print_channel(channel);
    uart_printf("\n");
}

void cmd_safety_set_channel(char* line, uint32_t length) {

    uint32_t channel;
    char key_name[64];
    char key_value[64];
    sscanf(line, "%i %s %s", &channel, &key_name, &key_value);

    safety_set_channel(channel, key_name, key_value);
    uart_printf("\n");
}

void cmd_calibration_read_channel(char* line, uint32_t length) {
    uint32_t channel;

    sscanf(line, "%i", &channel);

    uart_printf(">%i ", channel);
    calibration_print_channel(channel);
    uart_printf("\n");
}

void cmd_calibration_set_channel(char* line, uint32_t length) {
    uint32_t channel;
    calibration_data_t new_calibration;

    sscanf(line, "%i %g %g %g %g %g %g %g %g %g",\
        &channel,\
        &new_calibration.current_scale,\
        &new_calibration.current_offset,\
        &new_calibration.temperature_scale,\
        &new_calibration.temperature_offset,\
        &new_calibration.output_voltage_scale,\
        &new_calibration.output_voltage_offset,\
        &new_calibration.oven_voltage_scale,\
        &new_calibration.oven_voltage_offset,\
        &new_calibration.temperature_current_coefficient);

    calibration_set_channel(channel, &new_calibration);

    uart_printf(">ok\n");
}



