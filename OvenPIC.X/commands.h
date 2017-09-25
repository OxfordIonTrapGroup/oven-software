
#ifndef _COMMANDS_H    
#define _COMMANDS_H

#define CMD_ECHO                        "echo"
#define CMD_VERSION                     "version"

#define CMD_SET_PWM_DUTY                "set_pwm_duty"

#define CMD_ADC_STREAM                  "adc_stream_channels"
#define CMD_ADC_DECIMATE                "adc_set_decimation"
#define CMD_ADC_READ_LAST_CONVERSION    "adc_read_sample"
#define CMD_ADC_READ_LAST_CALIBRATED_DATA "adc_read_calibrated_sample"

#define CMD_FEEDBACK_SET_CONFIG         "fb_set_config"
#define CMD_FEEDBACK_GET_CONFIG         "fb_get_config"
#define CMD_FEEDBACK_START              "fb_start"
#define CMD_FEEDBACK_STOP               "fb_stop"
#define CMD_FEEDBACK_SETPOINT           "fb_set_setpoint"
#define CMD_FEEDBACK_READ_STATUS        "fb_read_status"
#define CMD_FEEDBACK_SET_LIMITS         "fb_set_limits"
#define CMD_FEEDBACK_GET_LIMITS         "fb_get_limits"

#define CMD_SETTINGS_LOAD "settings_load"
#define CMD_SETTINGS_SET_TO_FACTORY "settings_set_to_factory"
#define CMD_SETTINGS_SAVE "settings_save"
#define CMD_SETTINGS_PRINT "settings_print"

#define CMD_SAFETY_STATUS "safety_status"
#define CMD_SAFETY_READ_CHANNEL "safety_read_channel"
#define CMD_SAFETY_SET_CHANNEL "safety_set_channel"

#define CMD_CALIBRATION_READ_CHANNEL "calibration_read_channel"
#define CMD_CALIBRATION_SET_CHANNEL "calibration_set_channel"

#endif
