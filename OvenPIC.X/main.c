
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

////

void update_controllers() {

    fbc_update(current_controller);
    fbc_update(temperature_controller);
}

void main() {
    
    //ANSELA = 0;
    //ANSELB = 0;
    //SYSTEMConfig( SYSCLK, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);
    
    // Wait until the U1RX pin receives a signal
    TRISBbits.TRISB2 = 0;
    LATBbits.LATB2 = 0;
    
    while(PORTAbits.RA4 == 0);
    TRISBbits.TRISB2 = 1;
    TRISBbits.TRISB14 = 0;

    uart_config();

    adc_config();
    pwm_config();



    configure_current_controller();

    //INTEnableSystemMultiVectoredInt();

    while(1) {
        ins_read_next();
    }
}



