

#ifndef _INTERFACE_H    
#define _INTERFACE_H

typedef struct ins_header_s {
    uint8_t magic_start;
    uint8_t command;
    uint8_t len;
    uint8_t crc;
    uint8_t magic_end;
}ins_header_t;



extern void ins_send_reply(ins_header_t* header, void* reply, uint8_t length);
extern void ins_send_text_message(uint8_t command, uint8_t* format, ...);
extern void ins_report_error(uint8_t* format, ...);

extern void ins_read_next();

extern void ins_enable_streaming();
extern void ins_disable_streaming();

#endif


