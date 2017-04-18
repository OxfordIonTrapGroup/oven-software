
#include "HardwareProfile.h"

#include <stdint.h>
#include "AD7770.h"
#include "feedback.h"
#include "interface.h"

#define ADC_CLOCK_FREQ 8000000
#define ADC_RESET LATBbits.LATB5

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

void adc_streaming_start(uint8_t channels) {
    // Start streaming to uart
    if( channels == 0 ) {
        adc_streaming_stop();
    } else {
        ins_enable_streaming();
        streaming_decimation_counter = 0;
        streaming_channels = channels;
    }
}

void adc_streaming_stop() {
    streaming_channels = 0;
    
    ins_disable_streaming();
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
    uart_write(&streaming_channels, 1);
    
    for(i = 0; i < 8; i++) {
        if(streaming_channels & (1<<i)) {
            // Cat the data to the uart if the channel is enabled
            uart_write(&last_samples[i], 4);
        }
    }
}

void adc_config() {

    // Use Timer 2 and OC1 to generate ADC clock on RB3
    PR2 = (SYSCLK/ADC_CLOCK_FREQ) - 1;
    OC1RS = (PR2 + 1)*0.5;
    T2CONbits.ON = 1;
    OC1CONbits.OCM = 6;
    OC1CONbits.ON = 1;
    RPB3R = 0b101; // OC1 on RB3 (pin 7)
   
    // Configure ADC reset pin
    TRISBbits.TRISB5 = 0;
    ADC_RESET = 0;
    

    
    // Use SPI2
    // SCK2 - RB15 (26)
    // SDI2 - RB13 (24)
    // SDO2 - RB11 (22)
    // SS2 -  RB10 (21)
    // RESET - RB5(14)
    RPB11R = 0b0100; // SDO2
    RPB10R = 0b0100; // SS2
    SDI2R = 0b0011; // SDI2
    
    SPI2STATbits.SPIROV = 0; // Clear overflow flag
    SPI2BRG = 2; // SPI clkc is PBCLK / 4 (5MHz)
    
    SPI2CONbits.DISSDO = 0; // SDO enabled
    SPI2CONbits.CKP = 1; // Clock idles high
    SPI2CONbits.CKE = 0; // SPI mode 3
    SPI2CONbits.SMP = 1;
    SPI2CONbits.MSTEN = 1; // SPI master mode
    SPI2CONbits.MSSEN = 1; // SPI master SS enable
    SPI2CONbits.MODE32 = 0;
    SPI2CONbits.MODE16 = 1; // 16bit mode
    
    SPI2CONbits.ON = 1; // Enable SPI
    
    ADC_RESET = 1; // Take ADC out of reset
    
    uint32_t i;
    for(i=0;i<1000;i++); // Wait for a while
    
    adc_set_high_power();
    adc_set_reference_internal();
    //adc_set_decimation( 2048 );
    adc_set_decimation( 2000 );
    adc_enable_readout(1);
    
    // Configure interrupt on ADC sample read pin (RB10)
    INT3R = 0b0110;
    INTCONbits.INT3EP = 0; // Set polarity to falling edge   
    IFS0bits.INT3IF = 0; // Clear the flag
    IEC0bits.INT3IE = 1; // Enable the interrupt
    IPC0bits.INT3IP = 3; // Set the priority to 1

}

void __ISR( _EXTERNAL_3_VECTOR, IPL3AUTO) adc_ext_interrupt() {

    // Read the samples
    adc_read_samples(last_samples, last_samples_signed, last_samples_float);
    LED_GREEN = !LED_GREEN;
    
    // Update the feedback loop
    fb_update();

    if(streaming_channels != 0)
        adc_streaming_interrupt();
    
    IFS0bits.INT3IF = 0; // Clear the flag
}

char adc_read(char address) {
    while(SPI2STATbits.SPITBF == 1); // Wait for tx buffer to clear
    SPI2BUF = (0x80 | address) << 8;
    while(SPI2STATbits.SPIRBF == 0); // Wait for data transfer
    return SPI2BUF & 0xFF;
}

char adc_write(char address, char value) {
    while(SPI2STATbits.SPITBF == 1); // Wait for tx buffer to clear
    SPI2BUF = ((address) << 8) | (value & 0xFF);
  
    while(SPI2STATbits.SPIRBF == 0); // Wait for data transfer
    return SPI2BUF & 0xFF;
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
    adc_write(_SRC_N_LSB, (decimation & 0x00FF) >> 8);
    
    adc_write(_SRC_UPDATE, 0x01);
}

void adc_enable_readout(char enable) {
    char value;
    if(enable != 0)
        value = 0x90;
    else
        value = 0x80;
    adc_write(_GENERAL_USER_CONFIG_3, value);
    
    SPI2CONbits.MSSEN = 0; // SPI master SS disable
    RPB10R = 0b0000; // Take manual control of SS2 
    TRISBbits.TRISB10 = 0;
    LATBbits.LATB10 = 1;
}

void adc_read_samples(uint32_t* data, int32_t* data_signed, float* data_float) {
    int i;
    
    //SPI2CONbits.FRMCNT = 0b100; // Hold SS down for 8x2 16bit words
    //SPI2CONbits.FRMSYPW = 1; 
    //SPI2CONbits.FRMEN = 1; // Enable framed mode
    LATBbits.LATB10 = 0;

    for(i=0; i<8; i++) {
        while(SPI2STATbits.SPITBF == 1); // Wait for tx buffer to clear
        SPI2BUF = 0x8000;
        while(SPI2STATbits.SPIRBF == 0); // Wait for data transfer
        data[i] = (SPI2BUF & 0xFFFF) << 16;
        while(SPI2STATbits.SPITBF == 1); // Wait for tx buffer to clear
        SPI2BUF = 0x8000;
        while(SPI2STATbits.SPIRBF == 0); // Wait for data transfer
        data[i] |= (SPI2BUF & 0xFFFF);
    }
    LATBbits.LATB10 = 1;
        
    for(i=0; i<8; i++) {
        data_signed[i] = (data[i] & 0x7FFFFF);
        if(data[i] & (1<<23))
            data_signed[i] = data_signed[i] - 0x800000;

        data_float[i] = data_signed[i]/(0x800000*1.0);
    }
    
    //SPI2CONbits.FRMEN = 0; // Disable framed mode
    //SPI2CONbits.FRMCNT = 0b000; // Return SS to normal behaviour
    
    //return data;
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


