
#ifndef _PWM_H    
#define _PWM_H


extern void pwm_config();
extern void pwm_set_duty(float duty);

#define OVEN_PWM_PERIOD 199
#define OVEN_MAX_DUTY 0.3
#define PWM_DECIMATION_BITS 2


#endif
