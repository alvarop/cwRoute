/** @file main.c
*
* @brief Threaded packet handler
*
* @author Alvaro Prieto
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include "serial.h"
#include "routing.h"
#include "main.h"

#ifndef MAX_DEVICES
#define MAX_DEVICES (3)
#warning MAX_DEVICES not defined, defaulting to 3
#endif

void send_serial_message( uint8_t* packet_buffer, int16_t buffer_size );

pthread_t serial_thread;
pthread_t routing_thread;

// Routing and power tables (all in one array)
static volatile uint8_t rp_tables[MAX_DEVICES * 2];

// Table storing all device routes
static volatile uint8_t *routing_table = &rp_tables[0];

// Table storing all device transmit powers
static volatile uint8_t *power_table = &rp_tables[MAX_DEVICES];


int main( int argc, char *argv[] )
{   
  int32_t rc;
  
  // Handle interrupt events to make sure files are closed before exiting
  (void) signal( SIGINT, sigint_handler );

  // Make sure input is correct
  if( argc < 2 )
  {
    printf("Usage: %s port baudrate\n", argv[0]);
    return 0;
  }  
  
  printf("Threaded Serial Terminal\r\n");
  
  // Convert string serial number to integer and open port
  if( serial_open( atoi( argv[1] ), atoi( argv[2] ), &process_packet ) )
  {
    printf("Error opening serial port.\r\n");
    exit(-1);
  }
  
  // Initialize routing an power tables
  memset( (uint8_t*)power_table, 0xff, MAX_DEVICES );
  memset( (uint8_t*)routing_table, ( MAX_DEVICES + 1 ), MAX_DEVICES );
  
  if ( routing_initialize() )
  {
    printf("Error initializing routes.\n");
    exit(-1);
  }
  
  rc = pthread_create( &serial_thread, NULL, serial_read_thread, NULL );
  
  if (rc)
  {
    printf("Error creating serial thread\n");
    exit(-1);
  }
  
  rc = pthread_create( &routing_thread, NULL, compute_routes_thread, 
                                                        (void*) routing_table );
  
  if (rc)
  {
    printf("Error creating routing thread\n");
    exit(-1);
  }
  
  for(;;)
  {
    //uint8_t index;
    
    // Wait until routing is done
    pthread_mutex_lock ( &mutex_route_done );
    
    // Send new routes to AP
    send_serial_message( (uint8_t *)rp_tables, sizeof(rp_tables) );
    
    // Print routes and powers
    /*
    for( index = 0; index < MAX_DEVICES; index++ )
    {
      printf( "%d->%d ", index+1, routing_table[index] );
    }
    
    printf("\n");
    
    for( index = 0; index < MAX_DEVICES; index++ )
    {
      printf( "%02X ", power_table[index] );
    }
    printf("\n");*/
  }
    
  
  return 0;
}

/*******************************************************************************
 * @fn     void process_packet( uint8_t* buffer, uint32_t size )
 * @brief  Process incoming serial packet
 * ****************************************************************************/
void process_packet( uint8_t* buffer, uint32_t size )
{
  uint8_t rssi_table[MAX_DEVICES+1][MAX_DEVICES+1]; 
  
  if( size < sizeof(rssi_table) )
  {
    printf( "Received packet smaller than RSSI table\r\n" );
    return;
  }
  
  memcpy( rssi_table, buffer, sizeof(rssi_table) );
  
  parse_table( rssi_table );
  
  // Let the routing algorithm run
  pthread_mutex_unlock ( &mutex_route_start );    
  
  return;
}

/*******************************************************************************
 * @fn     void sigint_handler( int32_t sig ) 
 * @brief  Handle interrupt event (SIGINT) so program exits cleanly
 * ****************************************************************************/
void sigint_handler( int32_t sig ) 
{
    
    pthread_cancel( serial_thread );
    pthread_cancel( routing_thread );
    
    routing_finalize();
    
    // Close the serial port
    serial_close();

    printf("\nExiting...\n");
    exit(sig);
}

