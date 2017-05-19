
#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <stdint.h>
#include "feedback_controller.h"
#include "calibration.h"

typedef struct {

    float test_value;
    uint32_t test_int;

    // PID controller settings structures
    controller_settings_t controller_settings[N_MAX_CONTROLLERS];

    // Oven ADC calibration data structures
    calibration_data_t calibration_data[2];

} settings_t;

extern settings_t settings;


#endif
