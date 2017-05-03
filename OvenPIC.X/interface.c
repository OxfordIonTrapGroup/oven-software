
#include "HardwareProfile.h"

#include <stdint.h>
#include <stdarg.h>

#include "uart.h"
#include "AD7770.h"
#include "feedback.h"
#include "commands.h"
#include "interface.h"

#define INS_BUFFER_LEN 256
uint8_t instruction_buffer[INS_BUFFER_LEN];
int instruction_buffer_end = 0;

#define INS_MAGIC_START (uint8_t)0xA5
#define INS_MAGIC_END   (uint8_t)0x5A

uint8_t streaming = 0; // Non-zero if interface is in streaming mode
                        // where normal command responses are hidden

void ins_enable_streaming() {
    streaming = 1;
}

void ins_disable_streaming() {
    streaming = 0;
}

void ins_send_reply(ins_header_t* header, void* reply, uint8_t length) {
    
    // If we are streaming, don't send a reply
    if(streaming != 0)
        return;
    
    uint8_t message[INS_BUFFER_LEN];
    
    ins_header_t new_header;
    
    new_header.magic_start = INS_MAGIC_START;
    new_header.magic_end = INS_MAGIC_END;
    
    new_header.len = length;
    new_header.command = header->command;
    
    new_header.crc = 0;
    
    memcpy(&message[0], &new_header, sizeof(ins_header_t));
    memcpy(&message[sizeof(ins_header_t)], reply, length);
    uart_write(message, sizeof(ins_header_t) + length);
}

void ins_send_text_message(uint8_t command, uint8_t* format, ...) {
    // If we are streaming, don't send anything
    if(streaming != 0)
        return;

    // Send a text message as a function reply       
    va_list arg_list;
    uint8_t message[INS_BUFFER_LEN];
    ins_header_t header;

    va_start(arg_list, format);             // Prep arguments list
    vsprintf(message, format, arg_list);    // "Print" to buffer
    va_end(arg_list);                       // End handling of arguments list

    header.command = command;
    ins_send_reply(&header, message, strlen(message));
}

void ins_report_error(uint8_t* format, ...) {
    // If we are streaming, don't send anything
    if(streaming != 0)
        return;

    // Report an error to the uart with a message       
    va_list arg_list;
    uint8_t message[INS_BUFFER_LEN];
    ins_header_t header;
       
    va_start(arg_list, format);             // Prep arguments list
    vsprintf(message, format, arg_list);    // "Print" to buffer
    va_end(arg_list);                       // End handling of arguments list
   
    header.command = CMD_ERROR;
    
    ins_send_reply(&header, message, strlen(message));
}


void ins_process_packet(ins_header_t* header, char* data) {
    
    switch(header->command) {
        case CMD_ECHO: 
            cmd_echo(header, data); 
            break;
            /*
        // PWM commands
        case CMD_PWM_SET_DUTY: 
            cmd_pwm_set_duty(header, data); 
            break;     
            
        // ADC commands

        case CMD_ADC_STREAM: 
            cmd_adc_stream(header, data); 
            break;
        case CMD_ADC_DECIMATE: 
            cmd_adc_decimate(header, data); 
            break;
        case CMD_ADC_READ_LAST_CONVERSION: 
            cmd_adc_read_last_conversion(header, data); 
            break;  
            
        // Feedback commands
            
        case CMD_FEEDBACK_CONFIG: 
            cmd_feedback_config(header, data); 
            break;
        case CMD_FEEDBACK_START: 
            cmd_feedback_start(header, data); 
            break;
        case CMD_FEEDBACK_STOP: 
            cmd_feedback_stop(header, data); 
            break;
        case CMD_FEEDBACK_SETPOINT: 
            cmd_feedback_setpoint(header, data); 
            break;
        case CMD_FEEDBACK_READ_STATUS: 
            cmd_feedback_read_status(header, data);
            break;

        case CMD_FEEDBACK_SET_LIMITS:
            cmd_feedback_set_limits(header, data);
            break;
            */
        default:
            ins_report_error("bad command code");
            break;
    }
    
}


void ins_rebase_buffer(int index) {
    // Copy the contents of the buffer starting at 'index'
    //  back to the start of the buffer
    if(index == 0) // If we don't need to shift, then leave
        return;
    
    int new_index;
    int len = instruction_buffer_end - index;
    for(new_index = 0; new_index < len; new_index++) {
        instruction_buffer[new_index] = instruction_buffer[index + new_index];
    }
    instruction_buffer_end = len;
}

void ins_invalidate_buffer() {
    
    //ins_report_error("Buffer invalidated");
    instruction_buffer_end = 0;
}


void ins_read_next() {
    // Reads from the uart rx buffer and checks for an instruction
    
    instruction_buffer_end += uart_read(&instruction_buffer[instruction_buffer_end], INS_BUFFER_LEN-instruction_buffer_end);

    // Search for packet start symbol
    int i_start = 0;
    while(instruction_buffer[i_start] != INS_MAGIC_START) {
        i_start++;
        if(i_start >= instruction_buffer_end) {
            ins_invalidate_buffer();
            return; // If no start packet was found, invalidate the buffer and return
        }              
    }

    // Now we have found a start symbol
    // Check that there is enough data in the buffer to read the header
    if(i_start + sizeof(ins_header_t) > instruction_buffer_end) {
        // If header is not yet fully formed, discard the start of the buffer
        //  and return
        ins_rebase_buffer(i_start);
        return;
    }
    
    // Now we have a valid start symbol and enough length to see the header
    ins_header_t* header = (ins_header_t*)&instruction_buffer[i_start];
    // Check that the magic end symbol is correct
    if(header->magic_end != INS_MAGIC_END) {
        // If the end symbol is not correct, rebase the buffer to after this header
        // and leave
        ins_rebase_buffer(i_start + sizeof(ins_header_t));
        return;
    }
    
    // Now that the header is valid, check the length
    if(i_start + sizeof(ins_header_t) + header->len > instruction_buffer_end) {
        // If the end of the packet overshoots the buffer, then rebase and leave
        ins_rebase_buffer(i_start);
        return;
    }
    
    // Now we have a valid header and packet
    uint8_t* data = &instruction_buffer[i_start + sizeof(ins_header_t)];
    ins_process_packet(header, data);
    // Rebase to the end of the packet
    ins_rebase_buffer(i_start + sizeof(ins_header_t) + header->len);
}


