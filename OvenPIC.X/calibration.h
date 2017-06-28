
#ifndef _CALIBRATION_H
#define _CALIBRATION_H

#include <stdint.h>

// Structure containing calibration data for a set of oven channels (4 adcs)
// Calibration is of the form T = scale*V + offset
//  where V is the output of the ADC (-1 to 1)
//  and T is the dependent variable in the chosen units
typedef struct {
    float temperature_scale;
    float temperature_offset;

    float current_scale;
    float current_offset;

    float output_voltage_scale;
    float output_voltage_offset;

    float oven_voltage_scale;
    float oven_voltage_offset;

    // This is the apparent temperature reading offset when
    // current is applied to the oven.
    // Units are degrees C / A
    float temperature_current_coefficient;
} calibration_data_t;

// Structure containing samples which have been calibrated
typedef struct {
    float temperature;
    float current;
    float output_voltage;
    float oven_voltage;
} calibrated_values_t;

extern calibrated_values_t calibrated_oven[2];

extern void calibration_update_samples();
extern void calibration_print_channel(uint32_t channel);
extern void calibration_set_channel(uint32_t channel,\
    calibration_data_t* new_calibration);

#endif
