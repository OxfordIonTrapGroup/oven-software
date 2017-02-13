

#include <stdint.h> 
#include <am335x/sys_mcspi.h> 



extern volatile uint32_t* shared_mem;


/* P9.28 mcasp0_ahclkr.spi1_cs0, OUTPUT_PULLUP | MODE3 */
/* P9.29 mcasp0_fsx.spi1_d0, OUTPUT_PULLUP | MODE3 */
/* P9.30 mcasp0_axr0.spi1_d1, INPUT_PULLUP | MODE3 */
/* P9.31 mcasp0_aclkx.spi1_sclk, OUTPUT_PULLUP | MODE3 */

void set_master_mode() {

    CT_MCSPI1.MODULCTRL &= ~0x1; // SINGLE mode
    //CT_MCSPI1.MODULCTRL |= 0x1; // SINGLE mode

    CT_MCSPI1.CH0CONF &= ~0x3000; // TRM    
    CT_MCSPI1.CH0CONF |= 0 & 0x3000; // TXRX

    CT_MCSPI1.CH0CONF &= ~(0x40000 + 0x30000); // IS + DPE0/1    
    CT_MCSPI1.CH0CONF |= (0x60000); // IS + DPE1

}



void spi_test() {

    *((uint32_t*)0x44E00050) = 2; // Enable SPI1 clock    
    *((uint32_t*)0x44E00060) = 2; // Enable SPI clocks

    // Stop clocks from going idle
    //CT_MCSPI1.SYSCONFIG = 0x308;

    // Reset SPI
    CT_MCSPI1.SYSCONFIG |= 0x2;
    // Wait for reset to complete
    while((CT_MCSPI1.SYSSTATUS & 0x01) == 0); 


    //set_master_mode();
    // Configure the module
    CT_MCSPI1.MODULCTRL = 0x0;

    // Stop clocks from going idle
    //CT_MCSPI1.SYSCONFIG = 0x15;    
    CT_MCSPI1.SYSCONFIG = 0x208;

    CT_MCSPI1.SYST = 0x200; // Set D0 out and D1 in

    // Reset IRQs
    CT_MCSPI1.IRQSTATUS = 0xFFFFFFFF;

    // Disable interrupts
    CT_MCSPI1.IRQENABLE = 0x00000000;

    // Enable SPI1 ch0 with 32bit words
    //CT_MCSPI1.CH1CONF = 0x60fcb;
    // Enable SPI1 ch0 with 16bit words
    //CT_MCSPI1.CH0CONF = 0x607cb;    
    //CT_MCSPI1.CH0CONF = 0x607cb | (0xF<<2);
    // Enable SPI1 ch0 with 8bit words
    CT_MCSPI1.CH0CONF = 0x607cb;// | (1<<27);

    //shared_mem[1] = CT_MCSPI1.RX0;
//return;
    //while(1) {
        CT_MCSPI1.IRQSTATUS = 0xFFFFFFFF; // reset status bit
        // Enable channel
        CT_MCSPI1.CH0CTRL = 0x1; 
    //CT_MCSPI1.MODULCTRL |= 0x1;

        shared_mem[0] = CT_MCSPI1.CH0STAT;        
        // Wait until data has been transmitted
        //while((CT_MCSPI1.CH0STAT & 0x2) == 0);    
        CT_MCSPI1.IRQSTATUS = 0x1; // reset status bit

        __asm__ __volatile__(" LDI r8, 0x1234\n"
            " LDI32 r9, 0x481A0138\n"
            " SBBO &r8, r9, 0, 2\n"
        );

        __asm__ __volatile__(" LDI r8, 0x1234\n"
            " LDI32 r9, 0x481A0138\n"
            " SBBO &r8, r9, 0, 2\n"
        );
        //CT_MCSPI1.TX0 = 0x94008900;  
        //CT_MCSPI1.TX0 = 0x1100;  

        // Wait until data has been transmitted
        while((CT_MCSPI1.CH0STAT & 0x2) == 0);

        CT_MCSPI1.CH0CTRL = 0x0;         
CT_MCSPI1.CH0CTRL = 0x1; 
        CT_MCSPI1.TX0 = 0x94008900;  
        //CT_MCSPI1.CH0CTRL = 0x0; 



        

        uint32_t i=100000;
        while(i>0) i--;
    //}
    shared_mem[1] = CT_MCSPI1.CH0CTRL;

}


