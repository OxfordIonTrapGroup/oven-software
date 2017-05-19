
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "feedback_controller.h"
#include "settings.h"
#include "calibration.h"

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
    c->value = 0;
    c->error = 0;
    c->integrator = 0;

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
    c->value = c->value_getter();

    // Check if we should limit the output
    uint32_t limiting = fbc_check_limits(c);

    if(limiting == 0) {
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

    // Calculate the new control variable
    new_cv = c->error*c->s->p_gain + c->integrator;


    // Clip the new control variable to be in range
    if(new_cv > c->s->cv_limit_max)
        new_cv = c->s->cv_limit_max;
    else if(new_cv < c->s->cv_limit_min)
        new_cv = c->s->cv_limit_min;

    // Set the new control variable
    c->cv = new_cv;

    if(c->enabled)
        c->cv_setter(c->cv);
}


// Controller instances

#define CURRENT_LIMIT 10


controller_t* current_controller = NULL;

void current_controller_setter(float new_cv) {
    pwm_set_duty(1, new_cv);
}

float current_controller_getter() {
    return calibrated_oven[1].current;
}

void configure_current_controller() {

    // Configure the current feedback controller
    current_controller = fbc_init("current", 0);

    // Setter is the PWM duty cycle
    current_controller->cv_setter = current_controller_setter;
    current_controller->value_getter = current_controller_getter;

    // current_controller->p_gain = 0.01;
    // current_controller->i_gain = 0.01;

    // current_controller->cv_limit_max = 0.4;
    // current_controller->cv_limit_min = 0;

    // current_controller->value_limit_max = CURRENT_LIMIT;
}

//////

controller_t* temperature_controller = NULL;

// The temperature controller regulates the setpoint
// of the current controller
void temperature_controller_setter(float new_cv) {
    current_controller->setpoint = new_cv*new_cv;
}

float temperature_controller_getter() {
    return calibrated_oven[1].temperature;
}


void configure_temperature_controller() {
    temperature_controller = fbc_init("temperature", 1);

    temperature_controller->cv_setter = temperature_controller_setter;
    temperature_controller->value_getter = temperature_controller_getter;

    // temperature_controller->cv_limit_max = sqrt(CURRENT_LIMIT);
    // temperature_controller->cv_limit_min = 0;

    // temperature_controller->value_limit_max = 500;
}



void update_controllers() {

    fbc_update(current_controller);
    fbc_update(temperature_controller);
}
