/** @file serial.h
*
* @brief Serial port definitions and data types
*
* @author Alvaro Prieto
*/
#ifndef _SERIAL_H
#define _SERIAL_H

#include <stdint.h>

#define BUFFER_SIZE ( 512 )

// Function Prototypes
uint8_t serial_open( int32_t, int32_t, uint8_t (*)( uint8_t*, uint32_t) );
void *serial_read_thread();
void serial_close();

#endif /* _SERIAL_H */

