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
#include "main.h"

void send_serial_message( uint8_t* packet_buffer, int16_t buffer_size );

pthread_t serial_thread;

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
  
  rc = pthread_create( &serial_thread, NULL, serial_read_thread, NULL );
  
  if (rc)
  {
    printf("Error creating serial thread\n");
    exit(-1);
  }
  
  for(;;)
  {
    // Do nothing
    sleep(1);
  }
    
    

  return 0;
}

/*******************************************************************************
 * @fn     void process_packet( uint8_t* buffer, uint32_t size )
 * @brief  Process incoming serial packet
 * ****************************************************************************/
void process_packet( uint8_t* buffer, uint32_t size )
{
  uint16_t buffer_index;
  for( buffer_index = 0; buffer_index < size; buffer_index++ )
  {
    printf( "%02X ", buffer[buffer_index] );
  }
  printf("\r\n");
  
  return;
}

/*******************************************************************************
 * @fn     void sigint_handler( int32_t sig ) 
 * @brief  Handle interrupt event (SIGINT) so program exits cleanly
 * ****************************************************************************/
void sigint_handler( int32_t sig ) 
{
    
    pthread_cancel( serial_thread );
    
    // Close the serial port
    serial_close();

    printf("\nExiting...\n");
    exit(sig);
}

