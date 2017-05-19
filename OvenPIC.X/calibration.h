
#ifndef _CALIBRATION_H
#define _CALIBRATION_H


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
} calibration_data_t;

// Structure containing samples which have been calibrated
typedef struct {
    float temperature;
    float current;
    float output_voltage;
    float oven_voltage;
} calibrated_values_t;

extern calibration_data_t calibration_data[2];
extern calibrated_values_t calibrated_oven[2];

extern void calibration_update_samples();

#endif
