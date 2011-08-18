/** @file main.c
*
* @brief TinyOS serial packet handling functions
*
* @author Alvaro Prieto
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "main.h"
#include "rs232.h"

static int32_t serial_port_number;
static uint32_t time_counter = 0;
static FILE* main_fp;

// Table with all the RSSIs from all devices. This is sent back to the host.
static volatile uint8_t rssi_table[MAX_DEVICES+1][MAX_DEVICES+1];

int main( int argc, char *argv[] )
{   
  uint8_t serial_buffer[BUFFER_SIZE]; 
  uint8_t packet_buffer[BUFFER_SIZE];
  uint8_t final_buffer[BUFFER_SIZE];  
  //uint8_t adc_sample_buffer[ADC_MAX_SAMPLES];    
  uint16_t buffer_offset = 0;
  uint8_t new_packet = 0;
  uint16_t buffer_index;
  uint16_t bytes_read = 0;  
  
  // Handle interrupt events to make sure files are closed before exiting
  (void) signal( SIGINT, sigint_handler );

  // Make sure input is correct
  if( argc < 2 )
  {
    printf("Usage: %s port baudrate\n", argv[0]);
    return 0;
  }  
  
  printf("CC2500 Sniffer\r\n");
  
  // Convert string serial number to integer
  serial_port_number = atoi( argv[1] );
  
  // Use RS232 library to open serial port  
  if ( OpenComport( serial_port_number, atoi(argv[2]) ) )
  {
    
    printf("Error opening serial port.\n");
    return 1;
  }
  // Flush the port
  while( PollComport( serial_port_number, serial_buffer, BUFFER_SIZE ) );
  
  memset( serial_buffer, 0x00, sizeof(serial_buffer) );
  
  // Open capture file
  main_fp = fopen( "./rssi_tables.csv", "w" );
  
  // Run forever
  for(;;)
  {  
    // Read a buffer full of data (if available)
    bytes_read = PollComport( serial_port_number, serial_buffer, BUFFER_SIZE );
    if( bytes_read > 0 )
    {

      if( bytes_read >= BUFFER_SIZE )
      {
        printf("Uh oh!\n");
      }
      else
      {
        
        memcpy( packet_buffer + buffer_offset, serial_buffer, bytes_read );                        
       
        if ( packet_in_buffer( packet_buffer ) )
        {          
                             
          buffer_offset = 0;
          
          new_packet = 1;
          bytes_read = find_and_escape_packet( packet_buffer, final_buffer );          
          
        } 
        else
        {   
          
          buffer_offset += bytes_read;
          
          if( buffer_offset > BUFFER_SIZE )
          {
            printf("damn...\n");
            memset( packet_buffer, 0x00, sizeof(packet_buffer) );
            memset( serial_buffer, 0x00, sizeof(serial_buffer) );
            buffer_offset = 0;
          }
        }
        
        if( new_packet == 1 )
        {
          
          process_packet( final_buffer );
          
          new_packet = 0;
          memset( packet_buffer, 0x00, sizeof(packet_buffer) );
        }      

      }
    }
    memset( serial_buffer, 0x00, sizeof(serial_buffer) );
    
    // Don't take up all the processor time    
    usleep(20000);

  }  

  return 0;
}

void process_packet( uint8_t* buffer )
{
  uint8_t row_index;
  uint8_t col_index;
  
  memcpy( rssi_table, buffer, sizeof(rssi_table) );
  
  printf("   ");
  
  for( col_index = 0; col_index <= MAX_DEVICES; col_index++ )
  {
    printf(" %3d ", col_index );
  }
  
  printf("\r\n");
  
  // Print packet in hex
  for( row_index = 0; row_index <= MAX_DEVICES; row_index++ )
  { 
    printf("%2d ", row_index );
    
    for( col_index = 0; col_index < MAX_DEVICES; col_index++ )
    {
      printf("%+4d ", (int8_t)rssi_table[row_index][col_index] );
      fprintf( main_fp, "%d,", (int8_t)rssi_table[row_index][col_index] );
    }
    
    printf("%+4d", (int8_t)rssi_table[row_index][col_index] );
    fprintf( main_fp, "%d", (int8_t)rssi_table[row_index][col_index] );
    
    printf("\r\n");
    fprintf( main_fp, "\n" );
  }    

  printf("\r\n");
  fprintf( main_fp, "\n" );

  time_counter++;
  return;
}

/*!
  @brief Find a packet in the buffer
*/
uint8_t packet_in_buffer( uint8_t* old_buffer )
{
  // replace escaped characters
  uint16_t buffer_index = 0;
  uint8_t sync_bytes=0;

  // find first SYNC_BYTE
  while( (sync_bytes < 2) & (buffer_index < BUFFER_SIZE) )
  {
    if( old_buffer[buffer_index] == SYNC_BYTE )
    {
      sync_bytes++;      
    }
    buffer_index++;
  }
  
  // No packet found
  if ( sync_bytes < 2 )
  {
    return 0;
  }  
  
  return 1;  
}


/*!
  @brief Find a packet in the buffer, remove escape characters, and return size
*/
uint16_t find_and_escape_packet( uint8_t* old_buffer, uint8_t* new_buffer )
{
  // replace escaped characters
  uint16_t buffer_index = 0;
  uint16_t packet_size = 0;
  uint8_t start=0;

  // find first SYNC_BYTE
  while( (!start) & (buffer_index < BUFFER_SIZE) )
  {
    if( old_buffer[buffer_index] == SYNC_BYTE )
    {
      start = 1;
      // Don't include first sync byte for alignment issues
      //new_buffer[packet_size++] = old_buffer[buffer_index];
    }
    buffer_index++;
  }

  
  // process until next SYNC_BYTE is found
  while ( (start) & (buffer_index < BUFFER_SIZE) )
  {
    // Handle escape characters
    if( old_buffer[buffer_index] == ESCAPE_BYTE )
    {
      buffer_index++;
      old_buffer[buffer_index] ^= 0x20;
    }
    // Handle end character
    else if( old_buffer[buffer_index] == SYNC_BYTE )
    {
      start = 0;
    }
    new_buffer[packet_size++] = old_buffer[buffer_index];
    buffer_index++;
  }
  if( start == 1 )
  {
    return 0;
  }
  return packet_size;  
}

/*!
  @brief Handle interrupt event (SIGINT) so program exits cleanly
*/
void sigint_handler( int32_t sig ) 
{
    // Close the file
    fclose( main_fp );
    
    // Close the serial port
    CloseComport( serial_port_number );

    printf("\nExiting...\n");
    exit(sig);
}

