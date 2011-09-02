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

// Table storing all device transmit powers
static volatile uint8_t power_table[MAX_DEVICES];

// Table storing all device routes
static volatile uint8_t routing_table[MAX_DEVICES];

static const double rssi_values[256] = {
-72.0,-71.5,-71.0,-70.5,-70.0,-69.5,-69.0,-68.5,-68.0,-67.5,-67.0,
-66.5,-66.0,-65.5,-65.0,-64.5,-64.0,-63.5,-63.0,-62.5,-62.0,
-61.5,-61.0,-60.5,-60.0,-59.5,-59.0,-58.5,-58.0,-57.5,-57.0,
-56.5,-56.0,-55.5,-55.0,-54.5,-54.0,-53.5,-53.0,-52.5,-52.0,
-51.5,-51.0,-50.5,-50.0,-49.5,-49.0,-48.5,-48.0,-47.5,-47.0,
-46.5,-46.0,-45.5,-45.0,-44.5,-44.0,-43.5,-43.0,-42.5,-42.0,
-41.5,-41.0,-40.5,-40.0,-39.5,-39.0,-38.5,-38.0,-37.5,-37.0,
-36.5,-36.0,-35.5,-35.0,-34.5,-34.0,-33.5,-33.0,-32.5,-32.0,
-31.5,-31.0,-30.5,-30.0,-29.5,-29.0,-28.5,-28.0,-27.5,-27.0,
-26.5,-26.0,-25.5,-25.0,-24.5,-24.0,-23.5,-23.0,-22.5,-22.0,
-21.5,-21.0,-20.5,-20.0,-19.5,-19.0,-18.5,-18.0,-17.5,-17.0,
-16.5,-16.0,-15.5,-15.0,-14.5,-14.0,-13.5,-13.0,-12.5,-12.0,
-11.5,-11.0,-10.5,-10.0,-9.5,-9.0,-8.5,-136.0,-135.5,-135.0,
-134.5,-134.0,-133.5,-133.0,-132.5,-132.0,-131.5,-131.0,-130.5,-130.0,
-129.5,-129.0,-128.5,-128.0,-127.5,-127.0,-126.5,-126.0,-125.5,-125.0,
-124.5,-124.0,-123.5,-123.0,-122.5,-122.0,-121.5,-121.0,-120.5,-120.0,
-119.5,-119.0,-118.5,-118.0,-117.5,-117.0,-116.5,-116.0,-115.5,-115.0,
-114.5,-114.0,-113.5,-113.0,-112.5,-112.0,-111.5,-111.0,-110.5,-110.0,
-109.5,-109.0,-108.5,-108.0,-107.5,-107.0,-106.5,-106.0,-105.5,-105.0,
-104.5,-104.0,-103.5,-103.0,-102.5,-102.0,-101.5,-101.0,-100.5,-100.0,
-99.5,-99.0,-98.5,-98.0,-97.5,-97.0,-96.5,-96.0,-95.5,-95.0,
-94.5,-94.0,-93.5,-93.0,-92.5,-92.0,-91.5,-91.0,-90.5,-90.0,
-89.5,-89.0,-88.5,-88.0,-87.5,-87.0,-86.5,-86.0,-85.5,-85.0,
-84.5,-84.0,-83.5,-83.0,-82.5,-82.0,-81.5,-81.0,-80.5,-80.0,
-79.5,-79.0,-78.5,-78.0,-77.5,-77.0,-76.5,-76.0,-75.5,-75.0,
-74.5,-74.0,-73.5,-73.0,-72.5, };

void send_serial_message( uint8_t* packet_buffer, int16_t buffer_size );

int main( int argc, char *argv[] )
{   
  uint8_t serial_buffer[BUFFER_SIZE]; 
  uint8_t packet_buffer[BUFFER_SIZE];
  uint8_t final_buffer[BUFFER_SIZE];
  uint8_t tx_buffer[BUFFER_SIZE];  
  
  uint16_t buffer_offset = 0;
  uint8_t new_packet = 0;
 
  uint16_t bytes_read = 0;  
  
  memset((uint8_t*)power_table, 0xff, sizeof(power_table));
  memset((uint8_t*)routing_table, (MAX_DEVICES+1), sizeof(routing_table));
  
  routing_table[0] = 3; // route device 1 through device 3
  
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
          
          // send new routing table
          send_serial_message( (uint8_t *)routing_table, sizeof(routing_table) );
        }      

      }
    }
    memset( serial_buffer, 0x00, sizeof(serial_buffer) );
    
    // Don't take up all the processor time    
    usleep(20000);
    //

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
    printf(" %3d   ", col_index );
  }
  
  printf("\r\n");
  
  // Print packet in hex
  for( row_index = 0; row_index <= MAX_DEVICES; row_index++ )
  { 
    printf("%2d ", row_index );
    
    for( col_index = 0; col_index < MAX_DEVICES; col_index++ )
    {
      printf("%06.1f ", rssi_values[rssi_table[row_index][col_index]]  );
      fprintf( main_fp, "%d,", rssi_table[row_index][col_index] );
    }
    
    printf("%06.1f ", rssi_values[rssi_table[row_index][col_index]]  );
    fprintf( main_fp, "%d", rssi_table[row_index][col_index] );
    
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
  @brief Set-up outgoing packet's address and am_type
*/
void send_serial_message( uint8_t* packet_buffer, int16_t buffer_size )
{
  uint8_t p_tmp_buffer[BUFFER_SIZE];
  uint8_t total_size = 0;
  uint8_t buffer_index = 0;
 
  // Add start character
  p_tmp_buffer[total_size++] = SYNC_BYTE; 
   
  for(; buffer_size > 0; buffer_size-- )
  {
    if( ( SYNC_BYTE == packet_buffer[buffer_index] ) || 
        ( ESCAPE_BYTE == packet_buffer[buffer_index] ) )
    {
      p_tmp_buffer[total_size++] = ESCAPE_BYTE;
      p_tmp_buffer[total_size++] = packet_buffer[buffer_index++] ^ 0x20;
    }
    else
    {
      p_tmp_buffer[total_size++] = packet_buffer[buffer_index++];
    }
  }
  
  p_tmp_buffer[total_size++] = SYNC_BYTE;
  
  SendBuf(serial_port_number, p_tmp_buffer, total_size);
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

