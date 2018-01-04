#include "hardware_profile.h"
#include <stdint.h>

uint32_t sys_time;


void __ISR(_TIMER_4_VECTOR, IPL1AUTO) t4_interrupt() {
    IFS0bits.T4IF = 0; // Clear T4 interrupt
    sys_time += 1;
}


void timer_config() {
    // Generate a clock at 1kHz to update sys_time
    // PBCLK3 is 100MHz
    // Use /4 prescaler and count to 25000
    sys_time = 0;

    PR4 = 25000 - 1;
    IEC0bits.T4IE = 1; // Enable timer interrupt
    IFS0bits.T4IF = 0;
    IPC4bits.T4IP = 1;

    T4CONbits.TCKPS = 0b010; // 1:4 prescale (100 MHz => 25 MHz)
    T4CONbits.ON = 1;
}
