#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <prussdrv.h>
#include <pruss_intc_mapping.h>

#define PRU_NUM         0
#define AM33XX

int main (int argc, char **argv)
{
    unsigned int ret;
    tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;

    /* Initialize the PRU */
    prussdrv_init ();

    /* Open PRU Interrupt */
    ret = prussdrv_open(PRU_EVTOUT_0);
    if (ret) {
        printf("prussdrv_open open failed\n");
        return (ret);
    }

    /* Get the interrupt initialized */
    prussdrv_pruintc_init(&pruss_intc_initdata);

    volatile void* shared_mem_void = NULL;
    prussdrv_map_prumem( PRUSS0_SHARED_DATARAM, (void**) &shared_mem_void );
    volatile uint32_t* shared_mem_uint32 = (uint32_t*) shared_mem_void;




    /* Execute example on PRU */
    prussdrv_load_datafile(PRU_NUM, "./data.bin");
    prussdrv_exec_program (PRU_NUM, "./text.bin");

    /* Wait until PRU0 has finished execution */
    prussdrv_pru_wait_event (PRU_EVTOUT_0);
    prussdrv_pru_clear_event (PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);

    // Read shared mem
    printf("%x\n", shared_mem_uint32[0] );    
    printf("%x\n", shared_mem_uint32[1] );

    /* Disable PRU and close memory mapping*/
    prussdrv_pru_disable (PRU_NUM);
    prussdrv_exit ();

    return(0);
}
