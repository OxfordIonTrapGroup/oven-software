
#ifndef _PWM_H    
#define _PWM_H


extern void pwm_config();
extern void pwm_set_duty(uint16_t duty);

#define OVEN_PWM_PERIOD 199
#define OVEN_MAX_DUTY (int)(0.3*OVEN_PWM_PERIOD)


#endif
