
#ifndef _FEEDBACK_H    
#define _FEEDBACK_H


extern void fb_config(int32_t new_gain_p, int32_t new_gain_i, int32_t new_gain_d);
extern void fb_start();
extern void fb_stop();
extern void fb_set_setpoint(int32_t new_setpoint);
extern void fb_update();
extern void fb_read_status();

#endif
