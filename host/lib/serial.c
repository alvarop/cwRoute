/** @file serial.c
*
* @brief Serial port functions
*
* @author Alvaro Prieto
*/
#include <stdlib.h>
#include "serial.h"
#include "rs232.h"

static int32_t serial_port_number;

uint8_t packet_in_buffer( uint8_t* );
uint16_t find_and_escape_packet( uint8_t*, uint8_t* );
static void dummy_callback( uint8_t* buffer, uint32_t size );

static void (*serial_read_callback)( uint8_t*, uint32_t ) = dummy_callback;

#define SYNC_BYTE   ( 0x7E )
#define ESCAPE_BYTE ( 0x7D )

/*******************************************************************************
 * @fn     uint8_t serial_open( int32_t port_number, int32_t baud_rate,
 *                                     void (*callback)( uint8_t*, uint32_t) )
 * @brief  Open serial port
 * ****************************************************************************/
uint8_t serial_open( int32_t port_number, int32_t baud_rate,
                                       void (*callback)( uint8_t*, uint32_t) )
{
  serial_port_number = port_number;
  // Use RS232 library to open serial port
  if ( OpenComport( serial_port_number, baud_rate ) )
  {
    // Error opening serial port
    return 1;
  }

  // Set a callback function for receiving packets
  serial_read_callback = callback;

  return 0;
}

/*******************************************************************************
 * @fn     void serial_close()
 * @brief  Close serial port
 * ****************************************************************************/
void serial_close()
{
  CloseComport( serial_port_number );
}

/*******************************************************************************
 * @fn     void *serial_read_thread()
 * @brief  Run as thread. Polls serial port and builds packets. Calls
 *         serial_callback function with each new packet
 * ****************************************************************************/
void *serial_read_thread()
{
  uint8_t serial_buffer[BUFFER_SIZE];
  uint8_t packet_buffer[BUFFER_SIZE];
  uint8_t final_buffer[BUFFER_SIZE];

  uint16_t buffer_offset = 0;
  uint8_t new_packet = 0;

  uint16_t bytes_read = 0;

  printf("Starting serial read.\r\n");

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
        printf("Read more bytes than buffer can handle.\n");
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
            printf("Buffer has overflowed. Flushing.\n");
            memset( packet_buffer, 0x00, sizeof(packet_buffer) );
            memset( serial_buffer, 0x00, sizeof(serial_buffer) );
            buffer_offset = 0;
          }
        }

        if( new_packet == 1 )
        {

          serial_read_callback( final_buffer, (bytes_read-1) );

          new_packet = 0;
          memset( packet_buffer, 0x00, sizeof(packet_buffer) );

        }

      }
    }
    memset( serial_buffer, 0x00, sizeof(serial_buffer) );

    // Don't take up all the processor time
    usleep(2000);

  }

}

/*******************************************************************************
 * @fn     uint8_t packet_in_buffer( uint8_t* old_buffer )
 * @brief  Check if there is a packet in the buffer
 * ****************************************************************************/
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

/*******************************************************************************
 * @fn     uint16_t find_and_escape_packet( uint8_t* old_buffer,
 *                                                         uint8_t* new_buffer )
 * @brief  Find packet in buffer, remove escape characters, and return size
 * ****************************************************************************/
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
 * @fn     void send_serial_message( uint8_t* packet_buffer,
 *                                                        int16_t buffer_size )
 * @brief  Escape buffer and transmit through serial port
 * ****************************************************************************/
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

/*******************************************************************************
 * @fn    void dummy_callback( uint8_t* buffer, uint32_t size )
 * @brief  Dummy function so the callback function isn't initially NULL
 * ****************************************************************************/
void dummy_callback( uint8_t* buffer, uint32_t size )
{
  // Do nothing
}
