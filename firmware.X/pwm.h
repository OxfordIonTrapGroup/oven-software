#pragma once

extern void pwm_config();
extern void pwm_set_duty(uint32_t channel, float duty);
extern void pwm_enable(uint32_t channel);
extern void pwm_disable(uint32_t channel);
extern void pwm_shutdown();

extern uint16_t pwm_duty[2];

#define OVEN_PWM_PERIOD 499
#define OVEN_MAX_DUTY 0.4
#define PWM_DECIMATION_BITS 4
