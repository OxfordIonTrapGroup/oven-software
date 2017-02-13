
#include <plib.h>
#include <stdint.h>
#include "HardwareProfile.h"
#include "uart.h"
#include "AD7770.h"

#define INS_BUFFER_LEN 100
char instruction_buffer[INS_BUFFER_LEN];
int instruction_buffer_end = 0;

#define INS_MAGIC_START (char)0xA5
#define INS_MAGIC_END   (char)0x5A

#define CMD_ECHO        0x00
#define CMD_READ_LAST_CONVERSION  0x01
#define CMD_STREAM_ADC  0x02
    // args: 1 byte, each bit represents channel mask

typedef struct ins_header_s {
    char magic_start;
    char command;
    char len;
    char crc;
    char magic_end;
}ins_header_t __attribute__((packed));

void ins_invalidate_buffer() {
   
    instruction_buffer_end = 0;
}

void ins_process_packet(ins_header_t* header, char* data) {
    
    if(header->command == CMD_ECHO) {
        uart_write(data, header->len);
    } else if(header->command == CMD_READ_LAST_CONVERSION) {
        float floatData[8];
        char buffer[120];
        adc_convert_samples(last_samples, floatData);
        
        sprintf(&buffer[0],"%f %f %f %f \n", floatData[4], floatData[5], floatData[6], floatData[7]);
        uart_write(buffer,strlen(buffer));
    } else if(header->command == CMD_STREAM_ADC) {
        adc_streaming_start(data[0]);
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

void ins_read_next() {
    // Reads from the uart rx buffer and checks for an instruction
    
    instruction_buffer_end = uart_read(&instruction_buffer[instruction_buffer_end], INS_BUFFER_LEN-instruction_buffer_end);

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
    char* data = &instruction_buffer[i_start + sizeof(ins_header_t)];
    ins_process_packet(header, data);
    // Rebase to the end of the packet
    ins_rebase_buffer(i_start + sizeof(ins_header_t) + header->len);
}


