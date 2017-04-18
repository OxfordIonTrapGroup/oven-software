
#ifndef _FEEDBACK_CONTROLLER_H
#define _FEEDBACK_CONTROLLER_H


// Controller datastructure
typedef struct {
    // PID gains
    float p_gain;
    float i_gain;
    float d_gain;

    float setpoint; // Current target value
    float value; // Last actual value
    float error; // Last error
    float integrator; // Value of the integrator
    float cv; // Value of control variable

    float value_limit_max; // Upper value limit

    float cv_limit_max; // Max allowable value of control var
    float cv_limit_min; // Min allowable value of control var

    uint32_t enabled; // Nonzero is regulation in enabled

    void (*cv_setter)(float); // Pointer to cv setter function
    float (*value_getter)(); // Pointer to value getter function
} controller_t;


extern controller_t* fbc_init();
extern void fbc_enable(controller_t* c);
extern void fbc_disable(controller_t* c);
extern uint32_t fbc_check_limits(controller_t* c);
extern void fbc_update(controller_t* c);


#endif