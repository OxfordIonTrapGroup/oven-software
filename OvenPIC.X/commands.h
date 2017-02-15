
#ifndef _COMMANDS_H    
#define _COMMANDS_H

#define CMD_ECHO                        0x00

#define CMD_PWM_SET_DUTY                0x10

#define CMD_ADC_STREAM                  0x20
#define CMD_ADC_DECIMATE                0x21
#define CMD_ADC_READ_LAST_CONVERSION    0x22

#define CMD_FEEDBACK_CONFIG             0x30
#define CMD_FEEDBACK_START              0x31
#define CMD_FEEDBACK_STOP               0x32
#define CMD_FEEDBACK_SETPOINT           0x33
#define CMD_FEEDBACK_READ_STATUS        0x34



typedef struct ins_header_s {
    uint8_t magic_start;
    uint8_t command;
    uint8_t len;
    uint8_t crc;
    uint8_t magic_end;
}ins_header_t;


// PWM commands

typedef struct {
    uint16_t new_duty;
} __attribute__((packed)) cmd_pwm_set_duty_args_t;


// ADC commands

typedef struct {
    uint8_t channels;
} __attribute__((packed)) cmd_adc_stream_args_t;

typedef struct  {
    int32_t decimation;
} __attribute__((packed)) cmd_adc_decimate_args_t;

typedef struct  {
    int32_t samples[8];
} __attribute__((packed)) cmd_adc_read_last_conversion_reply_t;

// Feedback commands

#define CMD_FEEDBACK_CONFIG_MODE_CONSTANTDUTY          0x00
#define CMD_FEEDBACK_CONFIG_MODE_CONSTANTCURRENT       0x01
#define CMD_FEEDBACK_CONFIG_MODE_CONSTANTTEMPERATURE   0x02
#define CMD_FEEDBACK_CONFIG_MODE_CONSTANTVOLTAGE       0x03

typedef struct {
    //uint8_t mode; // Feedback mode
    int32_t gain_p;
    int32_t gain_i;
    int32_t gain_d;
} __attribute__((packed)) cmd_feedback_config_args_t;

typedef struct {
    int32_t setpoint;
} __attribute__((packed)) cmd_feedback_setpoint_args_t;


#endif
