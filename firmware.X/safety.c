#include <string.h>
#include <stdio.h>
#include "hardware_profile.h"
#include "calibration.h"
#include "settings.h"
#include "safety.h"
#include "pwm.h"
#include "timer.h"
#include "AD7770.h"

/*
Oven safety scheme:

* over-temperature shutdown:
    If any calibrated temperature sample is above the limit shutdown the PWM.
    This protects against thermocouple faults, temperature typos (!), and
    oscillating PI controllers.
* under-temperature shutdown:
    If any calibrated temperature sample is below the limit shutdown the PWM.
    This protects against thermocouple faults, and inverted thermocouples
    giving positive feedback.
* over-current shutdown:
    If any calibrated current sample is above the limit shutdown the PWM.
    This protects against oscillating PI controllers or miscalibrated
    temperature sensors.
* maximum on-time:
    If the time the duty-cycle has been non-zero is above the limit shutdown the
    PMW. This protects against communication bugs, the Beaglebone crashing, or
    any other upstream programming errors
* watchdog timer:
    The watchdog is kicked in the main processing loop. If it is not kicked 
    for more than 128 ms the PIC is reset. This protects against runaway code or
    infinite loops in the command handling logic.
* ADC CRC:
    If more than ADC_CRC_MAX_FAILURES ADC CRC failures occur during any one
    oven burn shutdown the PWM.
*/


// Clearing the watchdog requires a 16bit write
// to the top 16 bits on the WDTCON register
uint16_t* wd_clear_pointer;

uint32_t safety_error_records[2];
#define SAFETY_ERROR_OVERCURRENT     0x01
#define SAFETY_ERROR_OVERTEMPERATURE 0x02
#define SAFETY_ERROR_OVERTIME        0x04
#define SAFETY_ERROR_UNDERTEMPERATURE 0x08
#define SAFETY_ERROR_ADC_CRC         0x10

// Local variables used to record on-times
// When the duty cycle > 0, on_time_started is set to 1
//  and the sys_time is recorded into on_time_start_times
// If the sys_time > on_time_start_times + max on time
//  then the output is disabled and error is flagged
// If the duty cycle is set to 0 again, the on_time_started
//  is reset to 0
uint32_t on_time_started[2];
uint32_t on_time_start_times[2];

// Local variable used to record the total CRC failure count when any duty cycle
// goes above zero. If more than ADC_CRC_MAX_FAILURES occur during any one oven
// burn the interlock is tripped.
uint32_t adc_crc_failure_count_started;
#define ADC_CRC_MAX_FAILURES (1)


// Initialise the safety routines
void safety_config() {
    wd_clear_pointer = (uint16_t*)0xBF800800 + 1;

    // Initialise the watchdog timer
    // WD timer driver by LPRC which runs at 32.768kHz
    // when WD timer reaches 32(?) it triggers a reset
    // So, with RUNDIV 1:128 (=0x07) we have ~128ms reset period
    WDTCONSET = 0x8000;

    // Clear reset status flags
    RCON = 0;

    safety_error_records[0] = 0;
    safety_error_records[1] = 0;

    adc_crc_failure_count_started = 0;
}


// Update the watchdog timer
void safety_clear_watchdog() {
    asm volatile("di");
    *wd_clear_pointer = 0x5743;
    asm volatile("ei");
}


void safety_print_errors() {
    uint32_t i;

    for(i=0; i<2; i++) {
        uart_printf("channel %i:", i);
        if(safety_error_records[i] & SAFETY_ERROR_OVERCURRENT)
            uart_printf(" over-current");
        if(safety_error_records[i] & SAFETY_ERROR_OVERTEMPERATURE)
            uart_printf(" over-temperature");
        if(safety_error_records[i] & SAFETY_ERROR_OVERTIME)
            uart_printf(" over-time");
        if(safety_error_records[i] & SAFETY_ERROR_UNDERTEMPERATURE)
            uart_printf(" under-temperature");
        if(safety_error_records[i] & SAFETY_ERROR_ADC_CRC)
            uart_printf(" adc-crc");
        if(safety_error_records[i] == 0)
            uart_printf(" no-errors");
        uart_printf(", ");
    }
}


// Check for oven safety
// Should be called after ADC samples have been calibrated
void safety_check() {
    uint32_t i;

    for(i=0; i<2; i++) {
        // Check oven temperature against min and max limits
        if(!settings.safety_settings.oven_temperature_check_disabled[i]) {
            if(calibrated_oven[i].temperature > \
                settings.safety_settings.oven_temperature_max[i]) {
                safety_error_records[i] |= SAFETY_ERROR_OVERTEMPERATURE;
                pwm_disable(i);
            } else if(calibrated_oven[i].temperature < \
                settings.safety_settings.oven_temperature_min[i]) {
                safety_error_records[i] |= SAFETY_ERROR_UNDERTEMPERATURE;
                pwm_disable(i);
            }
        }

        // Check oven current against max
        if(!settings.safety_settings.oven_current_check_disabled[i]) {
            if(calibrated_oven[i].current > \
                settings.safety_settings.oven_current_max[i]) {
                safety_error_records[i] |= SAFETY_ERROR_OVERCURRENT;
                pwm_disable(i);
            }
        }

        // Check oven run-time against max
        if(!settings.safety_settings.on_time_check_disabled[i]) {
            // While the oven is turned on, increment the counter
            // and die if needed
            if(pwm_duty[i] > 0) {
                if(on_time_started[i] == 0) {
                    // Oven just turned on
                    on_time_started[i] = 1;
                    on_time_start_times[i] = sys_time;
                    adc_crc_failure_count_started = adc_crc_failure_count;
                } else {
                    uint32_t time_to_die = on_time_start_times[i] + \
                        settings.safety_settings.on_time_max[i];
                    if(sys_time >= time_to_die) {
                        safety_error_records[i] |= SAFETY_ERROR_OVERTIME;
                        pwm_disable(i);
                    }
                }
            } else {
                // If the output is off, then reset the on_time_started flag
                on_time_started[i] = 0;
            }
        }

        // ADC CRC failure check
        if(pwm_duty[i] > 0) {
            if(adc_crc_failure_count - adc_crc_failure_count_started > 
                    ADC_CRC_MAX_FAILURES) {
                pwm_disable(i);
                safety_error_records[i] |= SAFETY_ERROR_ADC_CRC;
            }
        }
    }
}


// Print out the safety parameters for a given channel
void safety_print_channel(uint32_t channel) {
    if(channel > 1) {
        uart_printf("Bad channel");
    }

    uart_printf("%i", channel);
    uart_printf(" %g %g %i", settings.safety_settings.oven_temperature_max[channel],\
        settings.safety_settings.oven_temperature_min[channel], \
        settings.safety_settings.oven_temperature_check_disabled[channel]);

    uart_printf(" %g %i", settings.safety_settings.oven_current_max[channel],\
        settings.safety_settings.oven_current_check_disabled[channel]);
    uart_printf(" %i %i", settings.safety_settings.on_time_max[channel],\
        settings.safety_settings.on_time_check_disabled[channel]);
    uart_printf(" %g", settings.safety_settings.duty_max[channel]);
}


// Set the given parameter
void safety_set_channel(uint32_t channel, char* key_name, char* key_value) {
    float value;
    sscanf(key_value, "%g", &value);

    uint32_t success = 1;

    if(strcmp(key_name, "oven_temperature_max") == 0) {
        settings.safety_settings.oven_temperature_max[channel] = value;
    }
    else if(strcmp(key_name, "oven_temperature_min") == 0) {
        settings.safety_settings.oven_temperature_min[channel] = value;
    }
    else if(strcmp(key_name, "oven_current_max") == 0) {
        settings.safety_settings.oven_current_max[channel] = value;
    }
    else if(strcmp(key_name, "on_time_max") == 0) {
        uint32_t value_int;
        if(value > 0)
            value_int = value;
        else
            value_int = 0;
        settings.safety_settings.on_time_max[channel] = value;
    }
    else if(strcmp(key_name, "duty_max") == 0) {

        if(value > 1)
            value = 1;
        else if(value < 0)
            value = 0;
        
        settings.safety_settings.duty_max[channel] = value;
    }

    else if(strcmp(key_name, "oven_temperature_check_disabled") == 0) {
        if(value == 0)
            settings.safety_settings.oven_temperature_check_disabled[channel] = 0;
        else
            settings.safety_settings.oven_temperature_check_disabled[channel] = 1;
    }
    else if(strcmp(key_name, "oven_current_check_disabled") == 0) {
        if(value == 0)
            settings.safety_settings.oven_current_check_disabled[channel] = 0;
        else
            settings.safety_settings.oven_current_check_disabled[channel] = 1;
    }
    else if(strcmp(key_name, "on_time_check_disabled") == 0) {
        if(value == 0)
            settings.safety_settings.on_time_check_disabled[channel] = 0;
        else
            settings.safety_settings.on_time_check_disabled[channel] = 1;
    }

    else {
        uart_printf("!Bad key name: %s", key_name);
        success = 0;
    }

    if(success == 1) {
        uart_printf(">success");
    }
}
