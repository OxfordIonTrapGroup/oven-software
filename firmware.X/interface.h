#pragma once

extern void ins_read_next();

extern void ins_enable_streaming();
extern void ins_disable_streaming();
extern void ins_process_line(char* data, uint32_t length);
extern void ins_unknown_command(char* data, uint32_t length);
