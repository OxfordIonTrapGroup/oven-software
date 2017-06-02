
#include "HardwareProfile.h"

#include <stdint.h>
#include <string.h>

#include "settings.h"
#include "feedback_controller.h"
#include "timer.h"

// Flash page size for pic32mz is 16kb
#define PAGE_SIZE 16*1024

// PIC32mz1024 has 1024 kb of flash. Bank 2 starts at 0xBD080000
volatile void* settings_in_flash = 0xBD080000; // Start of bank 2

settings_t settings;

// Set the settings to sane 'factory defaults'
void settings_set_to_factory() {

    uint32_t i;

    for(i=0;i<2;i++) {
        // Thermocouple has sensitivity of 40uV / C
        // Instrumentation amp has gain 51
        // Offset is the temperature of the cold junction ~20 C
        settings.calibration_data[i].temperature_scale = (2.5*(1000.0/40.0)*(1000.0/51.));
        settings.calibration_data[i].temperature_offset = 20;

        // Current sensor has a gain of 5 A / V
        settings.calibration_data[i].current_scale = 5*2.5;
        settings.calibration_data[i].current_offset = 0;

        // Output voltage sense has a gain of 3 V/V
        settings.calibration_data[i].output_voltage_scale = 3*2.5;
        settings.calibration_data[i].output_voltage_offset = 0;

        // Oven voltage sense has a gain of 3 V/V
        settings.calibration_data[i].oven_voltage_scale = 3*2.5;
        settings.calibration_data[i].oven_voltage_offset = 0;
    }

    for(i=0;i<N_MAX_CONTROLLERS;i++) {
        // PID gains
        settings.controller_settings[i].p_gain = 0;
        settings.controller_settings[i].i_gain = 0;
        settings.controller_settings[i].d_gain = 0;

        settings.controller_settings[i].value_limit_max = 0; // Upper value limit

        settings.controller_settings[i].cv_limit_max = 0; // Max allowable value of control var
        settings.controller_settings[i].cv_limit_min = 0; // Min allowable value of control var

        settings.controller_settings[i].default_setpoint = 0; // Default setpoint for controller
        // This is used only during initialisation
    }
}

// Read the setting strcuture from flash
void settings_read() {
    memcpy((void*)&settings, settings_in_flash, sizeof(settings_t));
}

// Write the setting structure to flash
void settings_write() {

    // Erase the page
    nvm_erase_page((void*)settings_in_flash);

    // Program the new settings
    nvm_program(
        (void*)settings_in_flash,
        &settings,
        sizeof(settings_t));

}

// Print out all the the settings
void settings_printout() {
    uint32_t i;

    uart_printf(">settings: ");

    for(i=0;i<2;i++) {
        uart_printf("TC%i %g,%g; ", i,
            settings.calibration_data[i].temperature_scale,
            settings.calibration_data[i].temperature_offset);

        uart_printf("I%i %g,%g; ", i,
            settings.calibration_data[i].current_scale,
            settings.calibration_data[i].current_offset);

        uart_printf("V_OUT%i %g,%g; ", i,
            settings.calibration_data[i].output_voltage_scale,
            settings.calibration_data[i].output_voltage_offset);

        uart_printf("V_OVEN%i %g,%g; ", i,
            settings.calibration_data[i].oven_voltage_scale,
            settings.calibration_data[i].oven_voltage_offset);
    }

    for(i=0;i<N_MAX_CONTROLLERS;i++) {
        uart_printf("FBC%i %g,%g,%g,%g,%g,%g,%g; ", i,
            settings.controller_settings[i].p_gain,
            settings.controller_settings[i].i_gain,
            settings.controller_settings[i].d_gain,
            settings.controller_settings[i].value_limit_max,
            settings.controller_settings[i].cv_limit_max,
            settings.controller_settings[i].cv_limit_min,
            settings.controller_settings[i].default_setpoint);
        //uart_printf(">t=%d", sys_time);
    }

        //uart_printf(">t=%d", sys_time);
    uart_printf("\n");
}

// Copied from reference manual (DS61193A)
void nvm_initiate_operation() {
    int int_status;

    // Disable interrupts
    asm volatile("di %0": "=r"(int_status));

    NVMKEY = 0;
    NVMKEY = 0xAA996655;
    NVMKEY = 0x556699AA;
    NVMCONSET = (1<<15); // Set the write bit in one operation

    uart_printf(">n %x ", NVMCON);

    // Restore interrupts
    if(int_status & 1)
        asm volatile("ei");
}


void nvm_clear_errors() {
    NVMCONbits.WREN = 1;
    NVMCONbits.NVMOP = 0; // Nop

    nvm_initiate_operation();

    // Wait for the WR bit to clear
    while(NVMCONbits.WR);

    NVMCONbits.WREN = 0;
    uart_printf(">c %x ", NVMCON);
}

// Erase a page of the NVM
void nvm_erase_page(void* page_address) {

    nvm_clear_errors();

    // Store the page address
    // Convert to a physical address
    NVMADDR = ((uint32_t)page_address) & 0x1fffffff;

    // Define the flash operation
    NVMCONbits.NVMOP = 0x04; // Page erase

    // Enable flash for write operation
    NVMCONbits.WREN = 1;

    nvm_initiate_operation();

    // Wait for the WR bit to clear
    while(NVMCONbits.WR);

    // Disable future write/erase operations
    NVMCONbits.WREN = 0;

    uart_printf(">%x %x ", NVMADDR, NVMCON);
}

// Program a word into the NVM
void nvm_program_word(void* destination, uint32_t value) {

    // Set up the address and data registers
    // Convert to a physical address
    NVMADDR = ((uint32_t)destination) & 0x1fffffff;
    NVMDATA0 = value;

    // Set the operation
    NVMCONbits.NVMOP = 0x01; // Word programming

    // Enable flash for write operation
    NVMCONbits.WREN = 1;

    nvm_initiate_operation();

    // Wait for the WR bit to clear
    while(NVMCONbits.WR);

    // Disable future write/erase operations
    NVMCONbits.WREN = 0;

    uart_printf(">v=%x %x %x %x", value, NVMADDR,
        ((uint32_t*)destination)[0], NVMCON);
}

// Program n_bytes of data from *data to the destination in the NVM
void nvm_program(void* destination, void* data, uint32_t n_bytes) {

    // Calculate the number of 4byte words that need to be programmed
    uint32_t n_words = (n_bytes / 4);
    if(n_bytes % 4)
        n_words += 1;

    uint32_t i;
    void* destination_pointer = destination;
    void* data_pointer = data;

    for(i=0; i < n_words; i++) {
        nvm_program_word(destination_pointer, *((uint32_t*)data_pointer));
        // Increment the pointers
        destination_pointer += 4;
        data_pointer += 4;
    }

}


