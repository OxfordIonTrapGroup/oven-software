
#include "HardwareProfile.h"

#include <stdint.h>
#include <string.h>

// Flash page size for pic32mz is 16kb
#define PAGE_SIZE 16*1024

typedef struct {

    float test_value;
    uint32_t test_int;
} settings_t;

#define SETTINGS_SIZE 16*1024

// PIC32mz1024 has 1024 kb of flash. Bank 2 starts at 0xBD080000
volatile void* settings_in_flash = 0xBD080000; // Start of bank 2

// volatile uint8_t settings_in_flash[SETTINGS_SIZE] \
//     __attribute__((aligned(PAGE_SIZE))) = {0xFF, 0xDD};
//     //__attribute__((aligned(PAGE_SIZE), section(".text,\"ax\", @progbits #"))) = {0xFF, 0xDD};

settings_t my_settings;

// Read the setting strcuture from flash
void settings_read() {
    memcpy((void*)&my_settings, settings_in_flash, sizeof(settings_t));
}

// Write the setting structure to flash
void settings_write() {

    //my_settings.test_value = 2.34;
    //my_settings.test_int = 501;

    // Erase the page
    nvm_erase_page((void*)settings_in_flash);

    // Program the new settings
    nvm_program(
        (void*)settings_in_flash,
        &my_settings,
        sizeof(settings_t));

}

void settings_printout() {

    uart_printf(">%f %i\n", my_settings.test_value, my_settings.test_int);
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


