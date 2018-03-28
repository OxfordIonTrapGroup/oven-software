#pragma once

#include <stdint.h>

typedef struct
{
    // Buffer base pointer
    uint8_t * buffBase;
    
    // Buffer length (in bytes)
    uint32_t length;
    
    // If non-zero, indicates that the buffer has overrun after the flag was last cleared
    uint8_t overrun;
    
    // Byte offsets into the buffer indicating the last position written and the next position to read out
    uint32_t inOffset;
    uint32_t outOffset;
}circBuff;



// Returns the number of bytes stored in the buffer 'buff'
uint32_t circBuffNBuffered( circBuff * buff );

// Discards all of the data in the buffer / initialises the buffer.
void circBuffFlush( circBuff * buff );

// Reads up to nBytes from 'buff'. If tmp is not null, stores up to nBytes into tmp. Returns the number of bytes read / removed from the buffer.
// Note that if there N bytes in the buffer, the number of bytes read will be min(N,nBytes)
int circBuffRead( circBuff * buff, uint8_t * tmp, int nBytes );

// Returns the i'th unread byte in the buffer, /but/ does not remove the bytes from the buffer. If their is no i'th unread byte in the buffer, null is returned.
uint8_t circBuffPeek( circBuff * buff, int i );

// Stores a byte in the buffer
void circBuffPush( circBuff * buff, uint8_t c );
