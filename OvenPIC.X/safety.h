

#ifndef _SAFETY_H
#define _SAFETY_H

#include <stdint.h>

typedef struct {

    // Maximum temperature the oven sensors are allowed to be.
    // If a temperature is read which exceeds these, and checking
    // is not disabled, then the pwm output is disabled.
    float oven_temperature_max[2];
    uint32_t oven_temperature_check_disabled[2];

    // Maximum current allowed to be driven through the oven.
    // If a current is read which exceeds these, and checking
    // is not disabled, then the pwm output is disabled.
    float oven_current_max[2];
    uint32_t oven_current_check_disabled[2];

    // After the oven has been powered for this duration, the
    // pwm output is disabled.
    float on_time_max[2];
    uint32_t on_time_check_disabled[2];

} safety_settings_t;

extern void safety_config();
extern void safety_clear_watchdog();
extern void safety_check();

#endif
