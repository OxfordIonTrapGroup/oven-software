
#ifndef _FEEDBACK_CONTROLLER_H
#define _FEEDBACK_CONTROLLER_H


typedef struct {
    // PID gains
    float p_gain;
    float i_gain;
    float d_gain;

    float value_limit_max; // Upper value limit

    float cv_limit_max; // Max allowable value of control var
    float cv_limit_min; // Min allowable value of control var

    float default_setpoint; // Default setpoint for controller
    // This is used only during initialisation

} controller_settings_t;

// Controller datastructure
typedef struct {

    // Standard PID settings for this controller
    controller_settings_t* s;

    float setpoint; // Current target value
    float value; // Last actual value
    float error; // Last error
    float integrator; // Value of the integrator
    float cv; // Value of control variable



    uint32_t enabled; // Nonzero is regulation in enabled

    void (*cv_setter)(float); // Pointer to cv setter function
    float (*value_getter)(); // Pointer to value getter function

    char* name; // Name string for this controller
} controller_t;


extern controller_t* fbc_init();
extern void fbc_enable(controller_t* c);
extern void fbc_disable(controller_t* c);
extern uint32_t fbc_check_limits(controller_t* c);
extern void fbc_update(controller_t* c);

extern controller_t* fbc_get_by_name(char* name);


// Maximum length of a feedback controller name
#define FBC_NAME_LEN 32

// Maximum possible number of feedback controllers
#define N_MAX_CONTROLLERS 5


#endif