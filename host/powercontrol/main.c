/** @file main.c
*
* @brief Power control test
*
* @author Alvaro Prieto
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include "main.h"
#include "rs232.h"

static int32_t serial_port_number;
static FILE* main_fp;

volatile uint8_t send_message = 1;
volatile uint8_t next_power = 0xff;

double tx_power;
double target_rssi;

double get_power_from_setting( uint8_t );

int main( int argc, char *argv[] )
{   
  uint8_t serial_buffer[BUFFER_SIZE]; 
  uint8_t packet_buffer[BUFFER_SIZE];
  uint8_t final_buffer[BUFFER_SIZE];  
  //uint8_t adc_sample_buffer[ADC_MAX_SAMPLES];    
  uint16_t buffer_offset = 0;
  uint8_t new_packet = 0;
 
  uint16_t bytes_read = 0;  
  
  // Handle interrupt events to make sure files are closed before exiting
  (void) signal( SIGINT, sigint_handler );

  tx_power = dbm_to_watt(1l);
  target_rssi = dbm_to_watt(-60l);

  // Make sure input is correct
  if( argc < 2 )
  {
    printf("Usage: %s port baudrate\n", argv[0]);
    return 0;
  }  
  
  printf("Power control test\r\n");
  
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
    if( 0 == send_message-- )
    {
      SendByte( serial_port_number, next_power ); // Send initial packet
      send_message = 1;
      
      if(next_power == 0xff)
      {
        printf("*");
      }
      // If no message is received before the next round, use full power
      next_power = 0xff;
    }
  }  

  return 0;
}

void process_packet( uint8_t* buffer )
{
  tx_power = dbm_to_watt(get_power_from_setting( buffer[0] ));
  
  // Compute alpha by dividing received rssi over transmit power  
  double alpha = dbm_to_watt(rssi_values[buffer[2]])/tx_power;
  
  // Compute required power by dividing target_rssi by alpha
  double required_power = watt_to_dbm(target_rssi/alpha);
  
  printf("Incoming RSSI %0.1f. Trying %0.1f to meet %0.1f rssi [0x%02X] Actual rssi: %0.1f\n", 
          rssi_values[buffer[2]], required_power, watt_to_dbm(target_rssi),
          find_closest_power(required_power), rssi_values[buffer[1]]);
  
  next_power = find_closest_power(required_power);
  //SendByte( serial_port_number, 0x00 );

  return;
}

double get_power_from_setting( uint8_t setting )
{ 
  uint8_t index;
  for( index = 0; index < sizeof(power_settings); index++ )
  {
    if( setting == power_settings[index] )
    {
      return power_values[index];
    }
  }
  
  printf("Power not found!\n");
  
  // In case the power isn't found, default to maximum
  return ( sizeof(power_values) - 1 );
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

/*******************************************************************************
 * @fn    uint8_t find_closest_power( double power )
 *
 * @brief Get a desired tx power in RSSI and returns radio register setting that
 *        matches as close as possible.
 * ****************************************************************************/
uint8_t find_closest_power( double power )
{
  uint8_t min_index = 0;
  uint8_t index;
  double min_value = 1e99;
  double difference;

  for( index = 0; index < (sizeof(power_values)/sizeof(double)); index++ )
  {
    
    difference = fabs( power - power_values[index] );
      
    if ( difference < min_value )
    {
      min_value = difference;
      min_index = index;
    }

  }
  
  //printf("%+0.1f", power_values[min_index]);
  
  return power_settings[min_index];
}

/*******************************************************************************
 * @fn    double dbm_to_watt( double power )
 *
 * @brief Convert power from dBm to Watts
 * ****************************************************************************/
double dbm_to_watt( double power )
{
  return pow(10, power/10l)/1000l;
}

/*******************************************************************************
 * @fn    double watt_to_dbm( double power )
 *
 * @brief Convert power from Watts to dBm
 * ****************************************************************************/
double watt_to_dbm( double power )
{
  return 10l * log10( 1000l * power );
}



/*!
  @brief Handle interrupt event (SIGINT) so program exits cleanly
*/
void sigint_handler( int32_t sig ) 
{
    
    // Close the serial port
    CloseComport( serial_port_number );

    printf("\nExiting...\n");
    exit(sig);
}

