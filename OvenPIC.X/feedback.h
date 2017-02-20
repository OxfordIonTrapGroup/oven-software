
#ifndef _FEEDBACK_H    
#define _FEEDBACK_H


extern int32_t FB_target;
extern uint8_t FB_adc_index;
extern int32_t FB_last_error; 
extern int64_t FB_last_duty; 
extern int64_t FB_integrator;
extern uint8_t FB_enabled;

extern int fb_check_outside_limits();
extern void fb_config(int32_t new_gain_p, int32_t new_gain_i, int32_t new_gain_d);
extern void fb_start();
extern void fb_stop();
extern void fb_set_setpoint(int32_t new_setpoint);
extern void fb_update();
extern void fb_read_status();
extern void fb_set_limits(int32_t limit_i, int32_t limit_t);

#endif
