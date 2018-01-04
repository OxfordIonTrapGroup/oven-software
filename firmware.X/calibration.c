#include <string.h>
#include "settings.h"
#include "calibration.h"
#include "AD7770.h"

#define CHANNEL_0_TEMPERATURE 3
#define CHANNEL_0_CURRENT 0
#define CHANNEL_0_OUTPUT_VOLTAGE 1
#define CHANNEL_0_OVEN_VOLTAGE 2

#define CHANNEL_1_TEMPERATURE 6
#define CHANNEL_1_CURRENT 5
#define CHANNEL_1_OUTPUT_VOLTAGE 4
#define CHANNEL_1_OVEN_VOLTAGE 7

calibrated_values_t calibrated_oven[2];


// Update the calibrated data values from last_samples_float
// This should be called directly after the ADC has been read out
void calibration_update_samples() {

    calibrated_oven[0].current = \
        settings.calibration_data[0].current_scale \
        * last_samples_float[CHANNEL_0_CURRENT] \
        + settings.calibration_data[0].current_offset;

    calibrated_oven[1].current = \
        settings.calibration_data[1].current_scale \
        * last_samples_float[CHANNEL_1_CURRENT] \
        + settings.calibration_data[1].current_offset;


    calibrated_oven[0].temperature = \
        (settings.calibration_data[0].temperature_scale \
        * last_samples_float[CHANNEL_0_TEMPERATURE]) \
        + settings.calibration_data[0].temperature_offset \
        + (settings.calibration_data[0].temperature_current_coefficient \
        * calibrated_oven[0].current);

    calibrated_oven[1].temperature = \
        (settings.calibration_data[1].temperature_scale \
        * last_samples_float[CHANNEL_1_TEMPERATURE]) \
        + settings.calibration_data[1].temperature_offset \
        + (settings.calibration_data[1].temperature_current_coefficient \
        * calibrated_oven[1].current);


    calibrated_oven[0].output_voltage = \
        settings.calibration_data[0].output_voltage_scale \
        * last_samples_float[CHANNEL_0_OUTPUT_VOLTAGE] \
        + settings.calibration_data[0].output_voltage_offset;

    calibrated_oven[1].output_voltage = \
        settings.calibration_data[1].output_voltage_scale \
        * last_samples_float[CHANNEL_1_OUTPUT_VOLTAGE] \
        + settings.calibration_data[1].output_voltage_offset;


    calibrated_oven[0].oven_voltage = \
        settings.calibration_data[0].oven_voltage_scale \
        * last_samples_float[CHANNEL_0_OVEN_VOLTAGE] \
        + settings.calibration_data[0].oven_voltage_offset;

    calibrated_oven[1].oven_voltage = \
        settings.calibration_data[1].oven_voltage_scale \
        * last_samples_float[CHANNEL_1_OVEN_VOLTAGE] \
        + settings.calibration_data[1].oven_voltage_offset;
}


void calibration_print_channel(uint32_t channel) {

    if(channel != 0)
        channel = 1;

    uart_printf("%g %g %g %g %g %g %g %g %g",\
        settings.calibration_data[channel].current_scale,\
        settings.calibration_data[channel].current_offset,\
        settings.calibration_data[channel].temperature_scale,\
        settings.calibration_data[channel].temperature_offset,\
        settings.calibration_data[channel].output_voltage_scale,\
        settings.calibration_data[channel].output_voltage_offset,\
        settings.calibration_data[channel].oven_voltage_scale,\
        settings.calibration_data[channel].oven_voltage_offset,\
        settings.calibration_data[channel].temperature_current_coefficient);
}


void calibration_set_channel(uint32_t channel,\
    calibration_data_t* new_calibration) {

    if(channel != 0)
        channel = 1;

    memcpy((void*)&settings.calibration_data[channel],\
        (void*)new_calibration, sizeof(calibration_data_t));

}
