#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "feedback_controller.h"
#include "settings.h"
#include "calibration.h"

// Sampling rate of ADC
#define ADC_SAMPLE_RATE 1000.0

// Array of pointers to controllers
controller_t* fbc_controllers[N_MAX_CONTROLLERS] = {NULL};
uint32_t n_fbc_controllers = 0; // Number of valid controller pointsers


// Initialise feedback controller, using the settings with index
controller_t* fbc_init(char* name, uint32_t index) {

    // If the settings index is out of bounds, fail
    if(index >= N_MAX_CONTROLLERS) {
        return NULL;
    }

    controller_t* c = malloc(sizeof(controller_t));

    c->s = &settings.controller_settings[index];

    c->setpoint = c->s->default_setpoint;
    c->target_setpoint = c->s->default_setpoint;
    c->value = 0;
    c->error = 0;
    c->integrator = 0;

    c->sample_decimation_average = 0;
    c->sample_decimation_counter = 0;

    c->limiting = 0;

    c->enabled = 0;

    c->name = name;

    // Store the reference to this controller
    fbc_controllers[index] = c;

    // Add the controller to the list
    if(n_fbc_controllers < N_MAX_CONTROLLERS) {
        fbc_controllers[n_fbc_controllers] = c;
        n_fbc_controllers += 1;
    }

    return c;
}


// Enable feedback controller
void fbc_enable(controller_t* c) {
    c->enabled = 1;
}


// Disable feedback controller
void fbc_disable(controller_t* c) {
    c->enabled = 0;
}


controller_t* fbc_get_by_name(char* name) {
    // Get a feedback controller by name
    uint32_t i;

    for(i=0; i < n_fbc_controllers; i++) {

        if(strcmp(name, fbc_controllers[i]->name) == 0) {
            // If the name matches the controller, return
            return fbc_controllers[i];
        }
    }

    // If no match was found, return NULL
    return NULL;
}


// Returns 1 if value is above max limit
// 0 otherwise
uint32_t fbc_check_limits(controller_t* c) {
    if(c->value > c->s->value_limit_max)
        return 1;
    return 0;
}


// Update the feedback control loop
void fbc_update(controller_t* c) {

    if(c->enabled == 0)
        return;

    // The value in c->value has been updated externally 'somehow'
    float new_value = c->value_getter();

    // Decimate if we need to
    if(c->s->sample_decimation > 0) {
        // Calculate the rolling average
        c->sample_decimation_average += \
            (new_value - c->value) / (c->s->sample_decimation + 1);

        // If we do not need to update the loop yet, return
        if(c->sample_decimation_counter < c->s->sample_decimation) {
            c->sample_decimation_counter++;
            c->value = new_value;
            return;
        } else { // Otherwise continue with loop updation
            c->sample_decimation_counter = 0;
        }
    }

    c->value = new_value;

    // Update the current setpoint if we are slew-rate limited
    if(c->setpoint != c->target_setpoint) {

        if(c->s->setpoint_slewrate == 0) {
            c->setpoint = c->target_setpoint;
        } else {
            float delta = c->target_setpoint - c->setpoint;
            float max_delta = c->s->setpoint_slewrate*(c->s->sample_decimation+1.0) /ADC_SAMPLE_RATE;

            if(abs(delta) > max_delta) {
                if(delta > 0)
                    c->setpoint += max_delta;
                else
                    c->setpoint -= max_delta;
            } else {
                c->setpoint = c->target_setpoint;
            }
        }
    }

    // Keep the last error value, for calculating the derivative
    float old_error = c->error;

    // Check if we should limit the output
    c->limiting = fbc_check_limits(c);

    if(c->limiting == 0) {
        // Calculate the error value
        c->error = c->setpoint - c->value;
    } else {
        c->error = c->s->value_limit_max - c->value;
    }

    float new_cv = 0;

    // If the control variable is saturated, don't add the 
    // integrator if the error is in the saturated direction
    if(c->cv >= c->s->cv_limit_max && c->error*c->s->i_gain > 0) {
        // Do nothing
    } else if(c->cv <= c->s->cv_limit_min && c->error*c->s->i_gain < 0) {
        // Do nothing
    } else {
        // Calculate the new integrator value
        c->integrator += c->error*c->s->i_gain;
    }

    // Calculate d component
    float d_value = 0;
    if(c->s->d_gain != 0) {
        d_value = c->s->d_gain*(c->error - old_error);
    }

    // Calculate the new control variable
    new_cv = c->error*c->s->p_gain + c->integrator + d_value;


    // Clip the new control variable to be in range
    if(new_cv > c->s->cv_limit_max) {
        new_cv = c->s->cv_limit_max;
    }
    else if(new_cv < c->s->cv_limit_min) {
        new_cv = c->s->cv_limit_min;
    }

    // Set the new control variable
    c->cv = new_cv;

    if(c->enabled)
        c->cv_setter(c->cv);
}




// Controller instances


controller_t* current_controller_0 = NULL;
controller_t* current_controller_1 = NULL;


void current_controller_setter_0(float new_cv) {
    pwm_set_duty(0, new_cv);
}


void current_controller_setter_1(float new_cv) {
    pwm_set_duty(1, new_cv);
}


float current_controller_getter_0() {
    return calibrated_oven[0].current;
}


float current_controller_getter_1() {
    return calibrated_oven[1].current;
}


void configure_current_controller_0() {

    // Configure the current feedback controller
    current_controller_0 = fbc_init("current_0", 0);

    // Setter is the PWM duty cycle
    current_controller_0->cv_setter = current_controller_setter_0;
    current_controller_0->value_getter = current_controller_getter_0;
}


void configure_current_controller_1() {

    // Configure the current feedback controller
    current_controller_1 = fbc_init("current_1", 1);

    // Setter is the PWM duty cycle
    current_controller_1->cv_setter = current_controller_setter_1;
    current_controller_1->value_getter = current_controller_getter_1;
}




//////

controller_t* temperature_controller_0 = NULL;
controller_t* temperature_controller_1 = NULL;


// The temperature controller regulates the setpoint
// of the current controller
void temperature_controller_setter_0(float new_cv) {
    current_controller_0->target_setpoint = new_cv*new_cv;
}


void temperature_controller_setter_1(float new_cv) {
    current_controller_1->target_setpoint = new_cv*new_cv;
}


float temperature_controller_getter_0() {
    return calibrated_oven[0].temperature;
}


float temperature_controller_getter_1() {
    return calibrated_oven[1].temperature;
}


void configure_temperature_controller_0() {
    temperature_controller_0 = fbc_init("temperature_0", 2);

    temperature_controller_0->cv_setter = temperature_controller_setter_0;
    temperature_controller_0->value_getter = temperature_controller_getter_0;
}


void configure_temperature_controller_1() {
    temperature_controller_1 = fbc_init("temperature_1", 3);

    temperature_controller_1->cv_setter = temperature_controller_setter_1;
    temperature_controller_1->value_getter = temperature_controller_getter_1;
}


void configure_controllers() {
    configure_current_controller_0();
    configure_current_controller_1();
    
    configure_temperature_controller_0();
    configure_temperature_controller_1();
}




/////////////////////


void update_controllers() {
    fbc_update(current_controller_0);
    fbc_update(current_controller_1);
    fbc_update(temperature_controller_0);
    fbc_update(temperature_controller_1);
}
