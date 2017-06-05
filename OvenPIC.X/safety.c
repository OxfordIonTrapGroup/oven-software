
#include "hardware_profile.h"
#include "calibration.h"
#include "settings.h"
#include "safety.h"
#include "pwm.h"

/*

Oven safety scheme:

-> over-temperature shutdown
-> over-current shutdown
-> maximum on-time
-> watchdog timer


*/


// Clearing the watchdog requires a 16bit write
// to the top 16 bits on the WDTCON register
uint16_t* wd_clear_pointer;

uint32_t safety_error_records[2];
#define SAFETY_ERROR_OVERCURRENT     0x01
#define SAFETY_ERROR_OVERTEMPERATURE 0x02
#define SAFETY_ERROR_OVERTIME        0x04


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

    //*wd_clear_pointer = 0x5743;

    safety_error_records[0] = 0;
    safety_error_records[1] = 0;
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

        if(!settings.safety_settings.oven_temperature_check_disabled[i]) {
            if(calibrated_oven[i].temperature > \
                settings.safety_settings.oven_temperature_max[i]) {
                safety_error_records[i] |= SAFETY_ERROR_OVERTEMPERATURE;
                pwm_disable(i);
            }
        }

        if(!settings.safety_settings.oven_current_check_disabled[i]) {
            if(calibrated_oven[i].current > \
                settings.safety_settings.oven_current_max[i]) {
                safety_error_records[i] |= SAFETY_ERROR_OVERCURRENT;
                pwm_disable(i);
            }
        }
    }
}
