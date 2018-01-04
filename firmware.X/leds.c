#include "hardware_profile.h"


// Initialise the LED port directions and states
void leds_config(void)
{
    // Configure the PCB status light
    LATEbits.LATE5 = 0;
    ANSELEbits.ANSE5 = 0;
    TRISEbits.TRISE5 = 0;

    // Configure the first channel status LEDs
    LATEbits.LATE6 = 0;
    LATEbits.LATE7 = 0;
    ANSELEbits.ANSE6 = 0;
    ANSELEbits.ANSE7 = 0;
    TRISEbits.TRISE6 = 0;
    TRISEbits.TRISE7 = 0;

    // Configure the second channel status LEDs
    LATEbits.LATE3 = 0;
    LATEbits.LATE4 = 0;
    ANSELEbits.ANSE4 = 0;
    TRISEbits.TRISE3 = 0;
    TRISEbits.TRISE4 = 0;
}


// Turn on / off the PCB status LED, mounted on the bottom left of the PCB
// This is not visible when the PCB is in the enclosure.
void leds_status_set(int state)
{
    LATEbits.LATE5 = (state != 0);
}


// Turn on / off the status LED associated with <channel>.
// Currently this only uses the 'A' channel of the LED connector, and leaves the
// 'B' channel at 0
void leds_channel_set(int channel, int state)
{
    if(channel==0) {
        LATEbits.LATE6 = (state != 0);
    }else if(channel==1) {
        LATEbits.LATE3 = (state != 0);
    }
}