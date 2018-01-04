
#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <stdint.h>
#include "feedback_controller.h"
#include "calibration.h"
#include "safety.h"

#define SETTINGS_MAGIC (0xdeadbeef)

typedef struct {
    float test_value;
    uint32_t test_int;

    // PID controller settings structures
    controller_settings_t controller_settings[N_MAX_CONTROLLERS];

    // Oven ADC calibration data structures
    calibration_data_t calibration_data[2];

    // Safety measure settings
    safety_settings_t safety_settings;

    // Should be SETTINGS_MAGIC
    uint32_t magic;
} settings_t;

extern settings_t settings;
extern void nvm_initiate_operation();
extern void nvm_clear_errors();
extern void nvm_erase_page(void* page_address);
extern void nvm_program_word(void* destination, uint32_t value);
extern void nvm_program(void* destination, void* data, uint32_t n_bytes);


#endif
