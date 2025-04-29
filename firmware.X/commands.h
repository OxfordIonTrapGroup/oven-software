#pragma once

#define CMD_ECHO                        "echo"
#define CMD_VERSION                     "version"

#define CMD_PWM_SET_DUTY                "pwm_set_duty"
#define CMD_PWM_GET_DUTY                "pwm_get_duty"
#define CMD_PWM_IS_ENABLED              "pwm_is_enabled"

#define CMD_ADC_STREAM                  "adc_stream_channels"
#define CMD_ADC_DECIMATE                "adc_set_decimation"
#define CMD_ADC_READ_LAST_CONVERSION    "adc_read_sample"
#define CMD_ADC_READ_LAST_CALIBRATED_DATA "adc_read_calibrated_sample"

#define CMD_FEEDBACK_SET_CONFIG         "fb_set_config"
#define CMD_FEEDBACK_GET_CONFIG         "fb_get_config"
#define CMD_FEEDBACK_START              "fb_start"
#define CMD_FEEDBACK_STOP               "fb_stop"
#define CMD_FEEDBACK_SETPOINT           "fb_set_setpoint"
#define CMD_FEEDBACK_SETPOINT_IMMEDIATE "fb_set_setpoint_immediate"
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


void cmd_echo(char* line, uint32_t length);
void cmd_version(char* line, uint32_t length);
void cmd_pwm_get_duty(char* line, uint32_t length);
void cmd_pwm_is_enabled(char* line, uint32_t length);
void cmd_pwm_set_duty(char* line, uint32_t length);
void cmd_adc_stream(char* line, uint32_t length);
void cmd_adc_decimate(char* line, uint32_t length);
void cmd_adc_read_last_conversion(char* line, uint32_t length);
void cmd_adc_read_last_calibrated_data(char* line, uint32_t length);
void cmd_feedback_set_config(char* line, uint32_t length);
void cmd_feedback_get_config(char* line, uint32_t length);
void cmd_feedback_start(char* line, uint32_t length);
void cmd_feedback_stop(char* line, uint32_t length);
void cmd_feedback_setpoint(char* line, uint32_t length);
void cmd_feedback_setpoint_immediate(char* line, uint32_t length);
void cmd_feedback_read_status(char* line, uint32_t length);
void cmd_feedback_set_limits(char* line, uint32_t length);
void cmd_feedback_get_limits(char* line, uint32_t length);
void cmd_settings_load(char* line, uint32_t length);
void cmd_settings_set_to_factory(char* line, uint32_t length);
void cmd_settings_save(char* line, uint32_t length);
void cmd_settings_print(char* line, uint32_t length);
void cmd_safety_status(char* line, uint32_t length);
void cmd_safety_read_channel(char* line, uint32_t length);
void cmd_safety_set_channel(char* line, uint32_t length);
void cmd_calibration_read_channel(char* line, uint32_t length);
void cmd_calibration_set_channel(char* line, uint32_t length);
