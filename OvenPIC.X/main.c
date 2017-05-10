
#include "HardwareProfile.h"

#include <math.h>
#include <stdint.h>
#include <string.h>
#include "AD7770.h"
#include "uart.h"
#include "feedback_controller.h"
#include "pwm.h"

#pragma config FPLLICLK = PLL_FRC // PLL source is FRC (8MHz)
#pragma config FPLLRNG = RANGE_5_10_MHZ
#pragma config FPLLIDIV = DIV_1 // 8/1 = 8 MHz
#pragma config FPLLMULT = MUL_50 // 8*50 = 400 MHz
#pragma config FPLLODIV = DIV_2  // 400/2 = 200 MHz

#pragma config FNOSC = SPLL
#pragma config FSOSCEN = OFF // Disable secondary oscillator
#pragma config OSCIOFNC = OFF // CLKO Output Signal Active on the OSCO Pin (Disabled)
#pragma config FDMTEN = OFF // Disable deadman timer

void ins_read_next();

#define CURRENT_LIMIT 10


controller_t* current_controller = NULL;

void current_controller_setter(float new_cv) {
    pwm_set_duty(1, new_cv);
}

float current_controller_getter() {
    return last_samples_float[5]*2.5*5;
}

void configure_current_controller() {

    // Configure the current feedback controller
    current_controller = fbc_init("current");

    // Setter is the PWM duty cycle
    current_controller->cv_setter = current_controller_setter;

    current_controller->value_getter = current_controller_getter;

    current_controller->p_gain = 0.01;
    current_controller->i_gain = 0.01;

    current_controller->cv_limit_max = 0.4;
    current_controller->cv_limit_min = 0;

    current_controller->value_limit_max = CURRENT_LIMIT;

}

//////

controller_t* temperature_controller = NULL;

// The temperature controller regulates the setpoint
// of the current controller
void temperature_controller_setter(float new_cv) {
    current_controller->setpoint = new_cv*new_cv;
}

float temperature_controller_getter() {
    return 2.5*last_samples_float[6]*(-1*(1000.0/40.0)*(1000.0/51.)) + 20;
}


void configure_temperature_controller() {
    temperature_controller = fbc_init("temperature");

    temperature_controller->cv_setter = temperature_controller_setter;
    temperature_controller->value_getter = temperature_controller_getter;



    temperature_controller->cv_limit_max = sqrt(CURRENT_LIMIT);
    temperature_controller->cv_limit_min = 0;

    temperature_controller->value_limit_max = 500;
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
    configure_temperature_controller();

    // Enable interrupts
    INTCONbits.MVEC = 1;
    asm volatile("ei");

    adc_set_streaming_decimation(100);

    while(1) {
        ins_read_next();
    }
}



