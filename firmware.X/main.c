#include "hardware_profile.h"

#include <math.h>
#include <stdint.h>
#include <string.h>
#include "AD7770.h"
#include "uart.h"
#include "feedback_controller.h"
#include "pwm.h"
#include "timer.h"
#include "leds.h"
#include "settings.h"
#include "safety.h"

#pragma config FPLLICLK = PLL_FRC // PLL source is FRC (8MHz)
#pragma config FPLLRNG = RANGE_5_10_MHZ
#pragma config FPLLIDIV = DIV_1 // 8/1 = 8 MHz
#pragma config FPLLMULT = MUL_50 // 8*50 = 400 MHz
#pragma config FPLLODIV = DIV_2  // 400/2 = 200 MHz

#pragma config FNOSC = SPLL
#pragma config FSOSCEN = OFF // Disable secondary oscillator
#pragma config OSCIOFNC = OFF // CLKO Output Signal Active on the OSCO Pin (Disabled)
#pragma config FDMTEN = OFF // Disable deadman timer
#pragma config WDTPS = 0x09// 512ms watchdog
#pragma config WINDIS = NORMAL // Watchdog in non-windowed mode
#pragma config FWDTEN = OFF
#pragma config WDTSPGM = STOP // Watchdog timer stopped during flash programming


void ins_read_next();


void main() {
    leds_config();
    timer_config();
    uart_config();
    safety_config();

    settings_read();

    adc_config();
    pwm_config();

    configure_controllers();
    
    // Enable interrupts
    INTCONbits.MVEC = 1;
    asm volatile("ei");

    adc_set_streaming_decimation(100);

    uint32_t last_time = 0;

    while(1) {
        // Clear the watchdog timer
        safety_clear_watchdog();

        if((sys_time % 1000) < 200) {
            leds_status_set(1);
        }else{
            leds_status_set(0);
        }

        ins_read_next();
    }
}



