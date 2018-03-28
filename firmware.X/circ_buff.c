#include "circ_buff.h"

#define NSTORED_CALC ( (buff->length + buff->inOffset - buff->outOffset) % buff->length )



// Returns the number of bytes stored in the buffer 'buf'
uint32_t circBuffNBuffered( circBuff * buff )
{
    uint32_t nStored;
    uint32_t interrupt_enabled = __builtin_get_isr_state();

    __builtin_disable_interrupts();
    nStored = NSTORED_CALC;
    __builtin_set_isr_state(interrupt_enabled);
    
    return nStored;
}


// Discards all of the data in the buffer / initialises the buffer.
void circBuffFlush( circBuff * buff )
{
    uint32_t interrupt_enabled = __builtin_get_isr_state();
    
    __builtin_disable_interrupts();
    
    buff->inOffset = 0;
    buff->outOffset = 0;
    
    buff->overrun = 0;
    
    __builtin_set_isr_state(interrupt_enabled);
}
 

// Reads up to nBytes from 'buff'. If tmp is not null, stores up to nBytes into tmp. Returns the number of bytes read / removed from the buffer.
// Note that if there N bytes in the buffer, the number of bytes read will be min(N,nBytes)
int circBuffRead( circBuff * buff, uint8_t * tmp, int nBytes )
{
    uint32_t interrupt_enabled = __builtin_get_isr_state();
    int nStored;
    int nToRead;
    int i;
    
    __builtin_disable_interrupts();
    
    nStored = NSTORED_CALC;
    nToRead = (nBytes>nStored)?(nStored):(nBytes);
    
    for( i=0; i<nToRead; i++) {
        if( (uint32_t)tmp != 0 ) tmp[i] = buff->buffBase[buff->outOffset];
        buff->outOffset = (buff->outOffset + 1) % buff->length;
    }
    
    __builtin_set_isr_state(interrupt_enabled);
    
    return nToRead;
}


// Returns the i'th unread byte in the buffer, /but/ does not remove the bytes from the buffer. If their is no i'th unread byte in the buffer, null is returned.
uint8_t circBuffPeek( circBuff * buff, int i )
{
    uint32_t interrupt_enabled = __builtin_get_isr_state();
    uint8_t c;
    
    __builtin_disable_interrupts();
    
    if( i >= NSTORED_CALC ) c = 0;
    else c = buff->buffBase[ (buff->outOffset + i) % buff->length ];
    
    __builtin_set_isr_state(interrupt_enabled);
    
    return c;
}


// Stores a byte in the buffer
void circBuffPush( circBuff * buff, uint8_t c )
{
    uint32_t interrupt_enabled = __builtin_get_isr_state();
    
    __builtin_disable_interrupts();
    
    // If the buffer is full, move the 'out' pointer forward, dropping the oldest byte
    if( NSTORED_CALC == buff->length-1 ) {
        buff->outOffset = ( buff->outOffset + 1 ) % buff->length;
        buff->overrun = 1;
    }

    buff->buffBase[ buff->inOffset ] = c;   
    buff->inOffset = ( buff->inOffset + 1 ) % buff->length;

    __builtin_set_isr_state(interrupt_enabled);
}

