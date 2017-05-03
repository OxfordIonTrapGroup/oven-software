
#include "HardwareProfile.h"

#include <stdint.h>
#include <string.h>
#include "AD7770.h"
#include "uart.h"
#include "feedback_controller.h"
#include "pwm.h"

/*#pragma config FNOSC = PRIPLL // Primary oscillator
#pragma config POSCMOD = XT // 'XT' mode for xtals, ours is 8MHz
#pragma config FPLLIDIV = DIV_2 // Divide input clock by 2
#pragma config FPLLMUL = MUL_20 // Multiply input clock by 20
#pragma config FPLLODIV = DIV_2 // Divide by 2, giving 40 MHz sys clock
#pragma config FPBDIV = DIV_1 // Peripheral clock = sys clock
#pragma config FSOSCEN = OFF // Secondary oscillator diabled, for dig IO on SOCS pins

#pragma config JTAGEN = OFF // Disable jtag  (needed for pin 24 (RED LED))
#pragma config ICESEL = ICS_PGx1 // ICE comm via pin-set 1 (needed for pin 24)
*/

#pragma config FPLLICLK = 1 // PLL source is FRC (8MHz)
#pragma config FPLLRNG = RANGE_8_16_MHZ
#pragma config FPLLIDIV = DIV_1 // 8/1 = 8 MHz
#pragma config FPLLMULT = MUL_45 // 8*45 = 360 MHz
#pragma config FPLLODIV = DIV_2  // 360/2 = 180 MHz

#pragma config FNOSC = SPLL
#pragma config FSOSCEN = OFF // Disable secondary oscillator
#pragma config OSCIOFNC = OFF // CLKO Output Signal Active on the OSCO Pin (Disabled)

void ins_read_next();


///////

controller_t* current_controller = NULL;

float current_controller_getter() {
    return last_samples_float[5];
}

void configure_current_controller() {

    // Configure the current feedback controller
    current_controller = fbc_init();

    // Setter is the PWM duty cycle
    current_controller->cv_setter = pwm_set_duty;

    current_controller->value_getter = current_controller_getter;

    current_controller->cv_limit_max = OVEN_MAX_DUTY;
    current_controller->cv_limit_min = 0;

}

//////

controller_t* temperature_controller = NULL;

// The temperature controller regulates the setpoint
// of the current controller
void temperature_controller_setter(float new_cv) {
    current_controller->setpoint = new_cv;
}

float temperature_controller_getter() {
    return last_samples_float[6];
}


void configure_temperature_controller() {
    temperature_controller = fbc_init();

    temperature_controller->cv_setter = temperature_controller_setter;
    current_controller->value_getter = temperature_controller_getter;
}



void update_controllers() {

    fbc_update(current_controller);
    fbc_update(temperature_controller);
}

void main() {
    
    // Configure the status light
    ANSELEbits.ANSE5 = 0;
    TRISEbits.TRISE5 = 0;
    LATEbits.LATE5 = 1;
    
    uart_config();

    adc_config();
    pwm_config();



    configure_current_controller();

    // Enable interrupts
    INTCONbits.MVEC = 1;
    asm volatile("ei");

    long i;
    while(1) {
        ins_read_next();
        //uart_write_blocking("hhhaa\n",6);
        for(i=0;i<1000000;i++);
        //if(U1STAbits.URXDA)
        //    LATEbits.LATE5 = ~LATEbits.LATE5;     

    }
}



