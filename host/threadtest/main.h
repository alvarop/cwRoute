/** @file main.h
*
* @brief Main definitions and data types
*
* @author Alvaro Prieto
*/
#ifndef _MAIN_H
#define _MAIN_H

#include <stdint.h>


#define BUFFER_SIZE ( 512 )

// Function Prototypes
void process_packet( uint8_t*, uint32_t );

void sigint_handler( int32_t sig );

#endif
