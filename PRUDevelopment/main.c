
#include <stdint.h> 
#include <am335x/pru_cfg.h> 

#define PRU0_ARM_INTERRUPT 19  // Interrupt used to signal end of PRU execution to host

volatile register uint32_t __R30;  // Output pins
volatile register uint32_t __R31;  // Input pins

volatile pruCfg CT_CFG __attribute__((cregister("PRU_CFG", near), peripheral));  

volatile uint32_t* shared_mem = (void*)0x10000;


 



void main() {

    // Enable the OCP master ports
    CT_CFG.SYSCFG_bit.STANDBY_INIT = 0;


    shared_mem[0] = 5;

    spi_test();

   
    __R31 = (__R31 & ~0xff) | (PRU0_ARM_INTERRUPT+16);

}


