/** @file main.h
*
* @brief Main definitions and data types
*
* @author Alvaro Prieto
*/
#ifndef _MAIN_H
#define _MAIN_H

#include <stdint.h>
//#include <unistd.h>

// File path
#define FILE_PATH "./"

#define BUFFER_SIZE ( 512 )

#define SYNC_BYTE   ( 0x7E )
#define ESCAPE_BYTE ( 0x7D )

#define MAX_DEVICES (6)

#define BROADCAST_ADDRESS (0x00)

#define RADIO_BUFFER_SIZE (64)

#define TYPE_MASK (0x0F)
#define FLAG_MASK (0xF0)

#define PACKET_START  (0x1)
#define PACKET_POLL   (0x2)
#define PACKET_SYNC   (0x3)

// The ACK flag is used to acknowledge a packet
#define FLAG_ACK    ( 0x10 )

// The BURST flag is used to identify a multi-packet transmission
#define FLAG_BURST  ( 0x20 )

// Function Prototypes
void process_packet( uint8_t* buffer );
uint8_t packet_in_buffer( uint8_t* );
uint16_t find_and_escape_packet( uint8_t*, uint8_t* );

inline uint16_t endian_swap16(uint16_t );
inline uint32_t endian_swap32(uint32_t );

void sigint_handler( int32_t sig );

#endif
