
#include "HardwareProfile.h"

#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "uart.h"
#include "commands.h"
#include "interface.h"

#define INS_BUFFER_LEN 256
char instruction_buffer[INS_BUFFER_LEN] = {0};
int instruction_buffer_end = 0;


uint8_t streaming = 0; // Non-zero if interface is in streaming mode
                       // where normal command responses are hidden

/*void ins_enable_streaming() {
    streaming = 1;
}

void ins_disable_streaming() {
    streaming = 0;
}*/

void ins_unknown_command(char* data, uint32_t length) {

    uart_printf("! Unknown command: %s\n", data);
}

void ins_process_line(char* data, uint32_t length) {

    char data_buffer[INS_BUFFER_LEN + 1];

    // Copy the data into a local buffer
    memcpy(data_buffer, data, length);

    // Null terminate the data
    data_buffer[length] = 0;

    // Convert the line to lower case
    uint32_t i;
    for(i=0; i<length; i++)
        data_buffer[i] = tolower(data_buffer[i]);

    char* command;

    // Find the command
    command = strtok(data_buffer, " ");

    // Find the length of the command
    uint32_t command_length = strlen(command);

    // Pointer to the rest of the line
    char* residual_line = command + command_length + 1;

    // Length of the rest of the line
    uint32_t residual_line_length = length - command_length;

    if(strcmp(command, CMD_ECHO) == 0) {
        cmd_echo(residual_line, residual_line_length);

    } else if(strcmp(command, CMD_SET_PWM_DUTY) == 0) {
        cmd_pwm_set_duty(residual_line, residual_line_length);

    } else if(strcmp(command, CMD_ADC_STREAM) == 0) {
        cmd_adc_stream(residual_line, residual_line_length);

    } else if(strcmp(command, CMD_ADC_DECIMATE) == 0) {
        cmd_adc_decimate(residual_line, residual_line_length);

    } else if(strcmp(command, CMD_ADC_READ_LAST_CONVERSION) == 0) {
        cmd_adc_read_last_conversion(residual_line, residual_line_length);

    } else if(strcmp(command, CMD_FEEDBACK_CONFIG) == 0) {
        cmd_feedback_config(residual_line, residual_line_length);

    } else if(strcmp(command, CMD_FEEDBACK_START) == 0) {
        cmd_feedback_start(residual_line, residual_line_length);

    } else if(strcmp(command, CMD_FEEDBACK_STOP) == 0) {
        cmd_feedback_stop(residual_line, residual_line_length);

    } else if(strcmp(command, CMD_FEEDBACK_SETPOINT) == 0) {
        cmd_feedback_setpoint(residual_line, residual_line_length);

    } else if(strcmp(command, CMD_FEEDBACK_READ_STATUS) == 0) {
        cmd_feedback_read_status(residual_line, residual_line_length);

    } else {
        ins_unknown_command(data_buffer, length);
    }

}

void ins_rebase_buffer(uint32_t index) {
    // Copy the contents of the buffer starting at 'index'
    //  back to the start of the buffer
    if(index == 0) // If we don't need to shift, then leave
        return;
    
    uint32_t new_index;
    uint32_t len = instruction_buffer_end - index;
    for(new_index = 0; new_index < len; new_index++) {
        instruction_buffer[new_index] = instruction_buffer[index + new_index];
    }
    instruction_buffer_end = len;
}

void ins_invalidate_buffer() {
    
    uart_printf("! Buffer invalidated\n");
    instruction_buffer_end = 0;
}


void ins_read_next() {
    // Reads from the uart rx buffer and checks for an instruction
    
    instruction_buffer_end += uart_read(&instruction_buffer[instruction_buffer_end], INS_BUFFER_LEN-instruction_buffer_end);

    // Search for new line
    uint32_t i_new_line = 0;
    while(instruction_buffer[i_new_line] != '\n') {
        i_new_line++;

        // If no new line was found in the whole buffer
        if(i_new_line >= INS_BUFFER_LEN) {
            // Throw is away
            ins_invalidate_buffer(); 
            return; 
        } else if(i_new_line >= instruction_buffer_end) {
            // Wait for more data
            return;
        }

    }

    // Now we have found a new line,
    char* data = &instruction_buffer[0];

    ins_process_line(data, i_new_line);
    // Rebase to the end of the line
    ins_rebase_buffer(i_new_line + 1);
}


