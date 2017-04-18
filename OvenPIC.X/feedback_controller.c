

#include <stdlib.h>
#include <stdint.h>

#include "feedback_controller.h"



// Initialise feedback controller
controller_t* fbc_init() {
    controller_t* c = malloc(sizeof(controller_t));

    c->p_gain = 0;
    c->i_gain = 0;
    c->d_gain = 0;

    c->setpoint = 0;
    c->value = 0;
    c->error = 0;
    c->integrator = 0;

    c->enabled = 0;

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

// Returns 1 if value is above max limit
// 0 otherwise
uint32_t fbc_check_limits(controller_t* c) {
    if(c->value > c->value_limit_max)
        return 1;
    return 0;
}

// Update the feedback control loop
void fbc_update(controller_t* c) {

    // The value in c->value has been updated externally 'somehow'
    c->value = c->value_getter();

    // Check if we should limit the output
    uint32_t limiting = fbc_check_limits(c);

    // Calculate the error value
    c->error = c->setpoint - c->value;

    float new_cv = 0;

    if(limiting == 0) {

        // If the control variable is saturated, don't add the 
        // integrator if the error is in the saturated direction
        if(c->cv >= c->cv_limit_max && c->error*c->i_gain > 0) {
            // Do nothing
        } else if(c->cv <= c->cv_limit_min && c->error*c->i_gain < 0) {
            // Do nothing
        } else {
            // Calculate the new integrator value
            c->integrator += c->error*c->i_gain;
        }

        // Calculate the new control variable
        new_cv = c->error*c->p_gain + c->integrator;
    } else {
        // If we are limiting, don't integrate, disable output
        c->integrator = 0;
        new_cv = c->cv_limit_min;
    }


    // Clip the new control variable to be in range
    if(new_cv > c->cv_limit_max)
        new_cv = c->cv_limit_max;
    else if(new_cv < c->cv_limit_min)
        new_cv = c->cv_limit_min;

    // Set the new control variable
    c->cv = new_cv;
    c->cv_setter(c->cv);
}
