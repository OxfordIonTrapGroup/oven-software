
#include "HardwareProfile.h"

#include <stdint.h>
#include "AD7770.h"
#include "feedback.h"
#include "interface.h"
#include "calibration.h"

#define ADC_RESET LATBbits.LATB14

#define _GENERAL_USER_CONFIG_1  0x11
#define _GENERAL_USER_CONFIG_2  0x12
#define _GENERAL_USER_CONFIG_3  0x13
#define _DOUT_FORMAT            0x14
#define _ADC_MUX_CONFIG         0x15
#define _GLOBAL_MUX_CONFIG      0x16
    
#define _GEN_ERR_REG_1      0x59
#define _GEN_ERR_REG_1_EN   0x5A
#define _GEN_ERR_REG_2      0x5B
#define _GEN_ERR_REG_2_EN   0x5C
#define _STATUS_REG_1       0x5D
#define _STATUS_REG_2       0x5E
#define _STATUS_REG_3       0x5F
#define _SRC_N_MSB          0x60
#define _SRC_N_LSB          0x61
#define _SRC_IF_MSB         0x62
#define _SRC_IF_LSB         0x63
#define _SRC_UPDATE         0x64

uint32_t last_samples[8]; // Storage of samples
int32_t last_samples_signed[8]; // Storage of samples
float last_samples_float[8];

uint32_t streaming_decimation = 0; // How many samples to skip before sending the next
                                    // sample whilst streaming
uint32_t streaming_decimation_counter = 0; // Counter for decimation

uint8_t streaming_channels = 0;

uint32_t adc_sample_index = 0; // Counter incremented every sample
uint32_t adc_crc_failure_count = 0; // Counter for CRC failures

void adc_streaming_start(uint8_t channels) {
    // Start streaming to uart
    if( channels == 0 ) {
        adc_streaming_stop();
    } else {
        //ins_enable_streaming();
        streaming_decimation_counter = 0;
        streaming_channels = channels;
    }
}

void adc_streaming_stop() {
    streaming_channels = 0;
    
    //ins_disable_streaming();
}

void adc_set_streaming_decimation(uint32_t decimation) {
    streaming_decimation = decimation;
}

void adc_streaming_interrupt() {
    int i;
    if(streaming_decimation > 0) {
        // If we need to decimate
        if(streaming_decimation_counter != 0) {
            // If the counter is not yet at zero, decrement and leave
            streaming_decimation_counter--;
            return;
        } else {
            // Otherwise, reset the counter and continue with the streaming
            streaming_decimation_counter = streaming_decimation;
        }
    }
    // Cat the streaming_channels byte
    uart_write_data(&streaming_channels, 1);

    // Cat the sample index
    //uart_write_data(&adc_sample_index, 4);

    for(i = 0; i < 8; i++) {
        if(streaming_channels & (1<<i)) {
            // Cat the data to the uart if the channel is enabled
            uart_write_data(&last_samples[i], 4);
        }
    }
}

void adc_config() {
    // ADC_MCLK is on RD5
    // SPI_CLK is on RD1 (SCK1)
    // SPI_MOSI is on RD10
    // SPI_MISO is on RD2
    // SPI_CS is on RD4
    // ADC_RESET is on RB14
    // ADC_DRDY is on RB15

    // ADC_MCLK:
    // Use Timer 2 and OC1 to generate ADC clock on RD5
    // PBCLK3 is 100 MHz, ADC_MCLK is 5 MHz
    PR2 = (100/5) - 1;
    OC1RS = (PR2 + 1)*0.5;
    T2CONbits.ON = 1;
    OC1CONbits.OCM = 6;
    OC1CONbits.ON = 1;
    TRISDbits.TRISD5 = 0;
    RPD5R = 0b1100; // OC1 on RD5
   
    // ADC_RESET:
    // On RB14
    ANSELBbits.ANSB14 = 0;
    TRISBbits.TRISB14 = 0;
    ADC_RESET = 0;
    
    // SPI_1:
    // SCK1 - RD1
    // SDI1 - RD2
    // SDO1 - RD10
    // SS1 -  RD4
    SDI1R = 0b0000; // SDI1 -> RD2
    RPD10R = 0b0101; // SDO1 -> RD10
    RPD4R = 0b0101; // SS1 -> RD4
    
    SPI1STATbits.SPIROV = 0; // Clear overflow flag
    // SPI clock is 25 MHz 
    SPI1BRG = 100; // PBCLK2 / 4 (100/4 = 25MHz)
    
    SPI1CONbits.DISSDO = 0; // SDO enabled
    SPI1CONbits.CKP = 1; // Clock idles high
    SPI1CONbits.CKE = 0; // SPI mode 3
    SPI1CONbits.SMP = 1;
    SPI1CONbits.MSTEN = 1; // SPI master mode
    SPI1CONbits.MSSEN = 1; // SPI master SS enable
    SPI1CONbits.MODE32 = 0;
    SPI1CONbits.MODE16 = 1; // 16bit mode
    
    SPI1CONbits.ON = 1; // Enable SPI
    
    ADC_RESET = 1; // Take ADC out of reset
    
    uint32_t i;
    for(i=0;i<1000;i++); // Wait for a while

    adc_set_high_power();
    adc_set_reference_internal();
    //adc_set_decimation( 2048 );
    adc_set_decimation(1250); // MCLK/(4*decimation) = 1 kHz 
    //adc_set_decimation(2000); // MCLK/(4*decimation) = 625 Hz 
    adc_enable_readout(1);
    
    // Configure interrupt on ADC sample read pin (RB15)
    ANSELBbits.ANSB15 = 0;
    INT2R = 0b0011;
    INTCONbits.INT2EP = 0; // Set polarity to falling edge   
    IFS0bits.INT2IF = 0; // Clear the flag
    IEC0bits.INT2IE = 1; // Enable the interrupt
    IPC3bits.INT2IP = 1; // Set the priority to 1

}

void __ISR( _EXTERNAL_2_VECTOR, IPL1AUTO) adc_ext_interrupt() {
    // Read the samples
    adc_read_samples(last_samples, last_samples_signed, last_samples_float);
    
    // Update the sample counter
    adc_sample_index += 1;
    // Update the feedback loop
    update_controllers();

    if(streaming_channels != 0)
        adc_streaming_interrupt();
    
    IFS0bits.INT2IF = 0; // Clear the flag
}

char adc_read(char address) {
    while(SPI1STATbits.SPITBF == 1); // Wait for tx buffer to clear
    SPI1BUF = (0x80 | address) << 8;
    while(SPI1STATbits.SPIRBF == 0); // Wait for data transfer
    return SPI1BUF & 0xFF;
}

char adc_write(char address, char value) {
    while(SPI1STATbits.SPITBF == 1); // Wait for tx buffer to clear
    SPI1BUF = ((address) << 8) | (value & 0xFF);
  
    while(SPI1STATbits.SPIRBF == 0); // Wait for data transfer
    return SPI1BUF & 0xFF;
}

void adc_set_high_power() {
    adc_write(_GENERAL_USER_CONFIG_1, 0x74);
}

void adc_set_reference_internal() {
    adc_write(_ADC_MUX_CONFIG, 0x58);
}

void adc_set_decimation(int decimation) {
    if ((decimation < 64) || (decimation > 2048))
        return; // bad decimation
    
    adc_write(_SRC_N_MSB, (decimation & 0xFF00) >> 8);
    adc_write(_SRC_N_LSB, (decimation & 0x00FF));
    
    adc_write(_SRC_UPDATE, 0x01);
}

void adc_enable_readout(char enable) {
    char value;
    if(enable != 0)
        value = 0x90;
    else
        value = 0x80;
    adc_write(_GENERAL_USER_CONFIG_3, value);
    
    SPI1CONbits.MSSEN = 0; // SPI master SS disable
    RPD4R = 0b0000; // Take manual control of SS2 
    TRISDbits.TRISD4 = 0;
    LATDbits.LATD4 = 1;
}

void adc_read_samples(uint32_t* data, int32_t* data_signed, float* data_float) {
    int i;
    uint32_t data_buffer[8];
    
    // Read in the raw data
    LATDbits.LATD4 = 0;
    for(i=0; i<8; i++) {
        while(SPI1STATbits.SPITBF == 1); // Wait for tx buffer to clear
        SPI1BUF = 0x8000;
        while(SPI1STATbits.SPIRBF == 0); // Wait for data transfer
        data_buffer[i] = (SPI1BUF & 0xFFFF) << 16;
        while(SPI1STATbits.SPITBF == 1); // Wait for tx buffer to clear
        SPI1BUF = 0x8000;
        while(SPI1STATbits.SPIRBF == 0); // Wait for data transfer
        data_buffer[i] |= (SPI1BUF & 0xFFFF);
    }
    LATDbits.LATD4 = 1;
        
    // Check for crc failure
    uint8_t crc_fails;
    crc_fails = adc_check_samples(data_buffer);

    if(crc_fails > 0) {
        // If the samples do no pass the CRC, then increment the failure count
        // and do not update the samples
        adc_crc_failure_count += 1;
    } else {
        // Otherwise, we are free to update the samples
        for(i=0; i<8; i++) {
            data[i] = data_buffer[i];
            data_signed[i] = (data_buffer[i] & 0x7FFFFF);
            if(data_buffer[i] & (1<<23))
                data_signed[i] = data_signed[i] - 0x800000;

            data_float[i] = data_signed[i]/(0x800000*1.0);
        }

        // Update the calibrated samples
        calibration_update_samples();
    }
}

uint8_t calculate_crc(uint32_t* data) {
    // Calculates the CRC of data[0] and data[1]
    
    uint8_t bData[7];
    
    bData[0] = (((data[0] >> 28) & 0x0F) << 4) + ((data[0] >> 20) & 0x0F);
    bData[1] = (data[0] >> 12);
    bData[2] = (data[0] >> 4);
    bData[3] = ((data[0] << 4) & 0xF0) + ((data[1] >> 28) & 0x0F);
    bData[4] = (data[1] >> 16);
    bData[5] = (data[1] >> 8);
    bData[6] = (data[1]);
    
    bData[0] = ~bData[0];
    
    uint8_t size = 7;
    uint8_t i = 0;
    uint8_t di = 0;
    uint16_t crc = 0;
    
    while(size > 0) {
        i = 0x80;
        while(i != 0) {
            if(((crc & 0x80) != 0) != ((bData[di] & i) != 0)) {
                crc = crc << 1;
                crc = crc ^ 0x107;
            } else {
                crc = crc << 1;
            }
            i = i >> 1;
        }
        di += 1;
        size -= 1;
    }
    return crc & 0xFF;
}
    
uint8_t adc_check_samples(uint32_t* data) {
    int i;
    uint8_t crc_fails = 0;
    
    for(i=0; i < 4; i++) {
        uint8_t crc_calc = calculate_crc(&data[2*i]);
        
        uint8_t crc_recv = ((data[2*i] >> 24) & 0x0F) << 4;
        crc_recv += ((data[2*i+1] >> 24) & 0x0F);
        
        if(crc_recv != crc_calc)
            crc_fails |= 1 << i;
    }
    return crc_fails;
}

void adc_convert_samples(uint32_t* data, float* floatData) {
    
    int i;
    int32_t value;
    
    for(i=0; i<8; i++) {
        value = (data[i] & 0xFFFFFF);
        if(value & (1<<23))
            value = (value&0x7FFFFF) - 0x800000;
        floatData[i] = value/(0x800000*1.0);
    }
}


