
#include "HardwareProfile.h"
#include "calibration.h"
#include "settings.h"
#include "safety.h"

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

// Initialise the safety routines
void safety_config() {

    wd_clear_pointer = (uint16_t*)WDTCON + 1;

    // Initialise the watchdog timer
    // WD timer driver by LPRC which runs at 32.768kHz
    // when WD timer reaches 32(?) it triggers a reset
    // So, with RUNDIV 1:128 (=0x07) we have ~128ms reset period
    WDTCONSET = 0x8000;

    // Clear reset status flags
    RCON = 0;
}

// Update the watchdog timer
void safety_clear_watchdog() {
    asm volatile("di");
    wd_clear_pointer = 0x5743;
    asm volatile("ei");
}

// Check for oven safety
// Should be called after ADC samples have been calibrated
void safety_check() {

    uint32_t i;

    for(i=0; i<2; i++) {

        if(!settings.safety_settings.oven_temperature_check_disabled[i]) {
            if(calibrated_oven[i].temperature > \
                settings.safety_settings.oven_temperature_max[i]) {

            }
        }

        if(!settings.safety_settings.oven_current_check_disabled[i]) {
            if(calibrated_oven[i].current > \
                settings.safety_settings.oven_current_max[i]) {

            }
        }
    }


}
