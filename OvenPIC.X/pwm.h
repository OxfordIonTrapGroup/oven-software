
#ifndef _PWM_H    
#define _PWM_H


extern void pwm_config();
extern void pwm_set_duty(uint32_t channel, float duty);

#define OVEN_PWM_PERIOD 499
#define OVEN_MAX_DUTY 0.4
#define PWM_DECIMATION_BITS 4


#endif
